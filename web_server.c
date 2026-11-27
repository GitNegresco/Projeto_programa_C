#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algoritmos/busca.h"
#include "algoritmos/ordenacao.h"
#include "modelos/entidade.h"
#include "persistencia/banco.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define CLOSE_SOCKET closesocket
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int Socket;
#define CLOSE_SOCKET close
#define INVALID_SOCKET_VALUE (-1)
#endif

#define MAX_REQUEST_BYTES (64U * 1024U)
#define MAX_FORM_FIELDS 32
#define MAX_FORM_KEY 64
#define MAX_FORM_VALUE 512
#define MAX_DATABASE_INT 2147483647L

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    int failed;
} Buffer;

typedef struct {
    char key[MAX_FORM_KEY];
    char value[MAX_FORM_VALUE];
} FormField;

typedef struct {
    FormField fields[MAX_FORM_FIELDS];
    int count;
} FormData;

typedef struct {
    const char *key;
    const char *value;
} TemplateValue;

static void buffer_init(Buffer *buffer)
{
    memset(buffer, 0, sizeof(*buffer));
}

static void buffer_free(Buffer *buffer)
{
    free(buffer->data);
    memset(buffer, 0, sizeof(*buffer));
}

static int buffer_reserve(Buffer *buffer, size_t additional)
{
    size_t needed;
    size_t capacity;
    char *data;

    if (buffer->failed || additional > (size_t)-1 - buffer->length - 1) {
        buffer->failed = 1;
        return 0;
    }
    needed = buffer->length + additional + 1;
    if (needed <= buffer->capacity) {
        return 1;
    }
    capacity = buffer->capacity == 0 ? 1024 : buffer->capacity;
    while (capacity < needed) {
        if (capacity > (size_t)-1 / 2) {
            capacity = needed;
            break;
        }
        capacity *= 2;
    }
    data = (char *)realloc(buffer->data, capacity);
    if (data == NULL) {
        buffer->failed = 1;
        return 0;
    }
    buffer->data = data;
    buffer->capacity = capacity;
    return 1;
}

static int buffer_append_n(Buffer *buffer, const char *text, size_t length)
{
    if (!buffer_reserve(buffer, length)) {
        return 0;
    }
    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return 1;
}

static int buffer_append(Buffer *buffer, const char *text)
{
    if (text == NULL) return 1;
    return buffer_append_n(buffer, text, strlen(text));
}

static int buffer_appendf(Buffer *buffer, const char *format, ...)
{
    va_list arguments;
    va_list copy;
    int length;

    va_start(arguments, format);
    va_copy(copy, arguments);
    length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (length < 0 || !buffer_reserve(buffer, (size_t)length)) {
        va_end(arguments);
        buffer->failed = 1;
        return 0;
    }
    vsnprintf(buffer->data + buffer->length,
              buffer->capacity - buffer->length, format, arguments);
    va_end(arguments);
    buffer->length += (size_t)length;
    return 1;
}

static int html_escape(Buffer *buffer, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)text;

    while (*cursor != '\0') {
        const char *replacement = NULL;
        switch (*cursor) {
        case '&': replacement = "&amp;"; break;
        case '<': replacement = "&lt;"; break;
        case '>': replacement = "&gt;"; break;
        case '"': replacement = "&quot;"; break;
        case '\'': replacement = "&#39;"; break;
        default: break;
        }
        if (replacement != NULL) {
            if (!buffer_append(buffer, replacement)) return 0;
        } else if (!buffer_append_n(buffer, (const char *)cursor, 1)) {
            return 0;
        }
        cursor++;
    }
    return 1;
}

static int replace_template(const char *source, const TemplateValue values[],
                            size_t value_count, Buffer *output)
{
    const char *cursor = source;

    while (*cursor != '\0') {
        const char *match = NULL;
        size_t match_index = 0;
        size_t i;

        for (i = 0; i < value_count; i++) {
            const char *candidate = strstr(cursor, values[i].key);
            if (candidate != NULL &&
                (match == NULL || candidate < match)) {
                match = candidate;
                match_index = i;
            }
        }
        if (match == NULL) {
            return buffer_append(output, cursor);
        }
        if (!buffer_append_n(output, cursor, (size_t)(match - cursor)) ||
            !buffer_append(output, values[match_index].value)) {
            return 0;
        }
        cursor = match + strlen(values[match_index].key);
    }
    return 1;
}

static int ascii_equal_nocase(const char *left, const char *right, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (tolower((unsigned char)left[i]) !=
            tolower((unsigned char)right[i])) {
            return 0;
        }
    }
    return 1;
}

static int url_decode(const char *source, size_t length, char *destination,
                      size_t capacity)
{
    size_t read_index;
    size_t write_index = 0;

    for (read_index = 0; read_index < length; read_index++) {
        unsigned char value = (unsigned char)source[read_index];
        if (value == '+') {
            value = ' ';
        } else if (value == '%') {
            int high;
            int low;
            unsigned char high_character;
            unsigned char low_character;
            if (read_index + 2 >= length) return 0;
            high_character = (unsigned char)source[read_index + 1];
            low_character = (unsigned char)source[read_index + 2];
            high = isdigit(high_character) ? high_character - '0' :
                (tolower(high_character) >= 'a' &&
                 tolower(high_character) <= 'f'
                    ? tolower(high_character) - 'a' + 10 : -1);
            low = isdigit(low_character) ? low_character - '0' :
                (tolower(low_character) >= 'a' &&
                 tolower(low_character) <= 'f'
                    ? tolower(low_character) - 'a' + 10 : -1);
            if (high < 0 || low < 0) return 0;
            value = (unsigned char)(high * 16 + low);
            read_index += 2;
        }
        if (value == '\0' || value < 0x20 || value == 0x7f ||
            write_index + 1 >= capacity) {
            return 0;
        }
        destination[write_index++] = (char)value;
    }
    destination[write_index] = '\0';
    return 1;
}

static int parse_form_data(const char *source, size_t length, FormData *form)
{
    size_t cursor = 0;
    memset(form, 0, sizeof(*form));

    while (cursor < length) {
        size_t end = cursor;
        size_t equals;
        FormField *field;

        while (end < length && source[end] != '&') end++;
        if (end == cursor || form->count >= MAX_FORM_FIELDS) return 0;
        equals = cursor;
        while (equals < end && source[equals] != '=') equals++;
        if (equals == cursor || equals == end) return 0;

        field = &form->fields[form->count];
        if (!url_decode(source + cursor, equals - cursor,
                        field->key, sizeof(field->key)) ||
            !url_decode(source + equals + 1, end - equals - 1,
                        field->value, sizeof(field->value))) {
            return 0;
        }
        form->count++;
        cursor = end + 1;
    }
    return 1;
}

static const char *form_value(const FormData *form, const char *key)
{
    int i;
    for (i = 0; i < form->count; i++) {
        if (strcmp(form->fields[i].key, key) == 0) {
            return form->fields[i].value;
        }
    }
    return "";
}

static int parse_positive_int(const char *text, int *value)
{
    char *end;
    long parsed;

    if (text == NULL || text[0] == '\0') return 0;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < 1 || parsed > MAX_DATABASE_INT) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

static int parse_optional_id(const char *text, int *value)
{
    if (text == NULL || text[0] == '\0' || strcmp(text, "-1") == 0) {
        *value = SEM_EQUIPE;
        return 1;
    }
    return parse_positive_int(text, value);
}

static int valid_text(const char *text, size_t max_length, int required)
{
    size_t length;
    size_t i;
    int has_non_space = 0;
    if (text == NULL) return 0;
    length = strlen(text);
    if (length > max_length) return 0;
    for (i = 0; i < length; i++) {
        if (!isspace((unsigned char)text[i])) {
            has_non_space = 1;
            break;
        }
    }
    return !required || has_non_space;
}

static int valid_phone(const char *text)
{
    size_t i;
    size_t length;
    if (text == NULL || text[0] == '\0') return 1;
    length = strlen(text);
    if (length > 20) return 0;
    for (i = 0; i < length; i++) {
        if (!isdigit((unsigned char)text[i])) return 0;
    }
    return 1;
}

static int parse_nonnegative_decimal(const char *text, double *value)
{
    char *end;
    double parsed;
    double cents;

    if (text == NULL || text[0] == '\0') return 0;
    errno = 0;
    parsed = strtod(text, &end);
    cents = parsed * 100.0;
    if (errno != 0 || end == text || *end != '\0' ||
        !isfinite(parsed) || parsed < 0.0 ||
        parsed > 99999999.99 ||
        fabs(cents - floor(cents + 0.5)) >= 1e-8) {
        return 0;
    }
    *value = parsed;
    return 1;
}

static int contains_nocase(const char *text, const char *query)
{
    size_t query_length;
    const char *cursor;
    if (query == NULL || query[0] == '\0') return 1;
    query_length = strlen(query);
    for (cursor = text; *cursor != '\0'; cursor++) {
        if (strlen(cursor) >= query_length &&
            ascii_equal_nocase(cursor, query, query_length)) {
            return 1;
        }
    }
    return 0;
}

static int append_options(Buffer *buffer, PGconn *connection,
                          const char *sql, int value_column,
                          int label_column)
{
    PGresult *result = PQexec(connection, sql);
    int i;

    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Falha ao carregar opções do formulário: %s",
                PQerrorMessage(connection));
        if (result != NULL) PQclear(result);
        return 0;
    }
    for (i = 0; i < PQntuples(result); i++) {
        if (!buffer_appendf(buffer, "<option value=\"%s\">",
                            PQgetvalue(result, i, value_column)) ||
            !html_escape(buffer, PQgetvalue(result, i, label_column)) ||
            !buffer_append(buffer, "</option>")) {
            PQclear(result);
            return 0;
        }
    }
    PQclear(result);
    return 1;
}

static int append_team_options(Buffer *buffer, const Equipe teams[],
                               int team_count)
{
    int i;
    for (i = 0; i < team_count; i++) {
        if (!buffer_appendf(buffer, "<option value=\"%d\">",
                            teams[i].id_equipe) ||
            !html_escape(buffer, teams[i].nome_equipe) ||
            !buffer_appendf(buffer, " (ID %d)</option>",
                            teams[i].id_equipe)) {
            return 0;
        }
    }
    return 1;
}

static int find_team_index(const Equipe teams[], int team_count, int team_id)
{
    return buscar_equipe_por_id(teams, team_count, team_id);
}

static int has_form_content_type(const char *request)
{
    const char *header_end = strstr(request, "\r\n\r\n");
    const char *line = strstr(request, "\r\n");
    if (header_end == NULL || line == NULL) return 0;
    line += 2;
    while (line < header_end) {
        const char *line_end = strstr(line, "\r\n");
        const char *colon;
        if (line_end == NULL || line_end > header_end) return 0;
        colon = memchr(line, ':', (size_t)(line_end - line));
        if (colon != NULL &&
            (size_t)(colon - line) == strlen("Content-Type") &&
            ascii_equal_nocase(line, "Content-Type",
                               strlen("Content-Type"))) {
            const char *value = colon + 1;
            const char *media_type = "application/x-www-form-urlencoded";
            size_t media_length = strlen(media_type);
            while (value < line_end && (*value == ' ' || *value == '\t')) {
                value++;
            }
            if ((size_t)(line_end - value) < media_length ||
                !ascii_equal_nocase(value, media_type, media_length)) {
                return 0;
            }
            value += media_length;
            return value == line_end || *value == ';' ||
                *value == ' ' || *value == '\t';
        }
        line = line_end + 2;
    }
    return 0;
}

static int build_classification(const Equipe teams[], int team_count,
                                Partida matches[], int match_count,
                                Classificacao ranking[])
{
    int i;
    for (i = 0; i < team_count; i++) {
        memset(&ranking[i], 0, sizeof(ranking[i]));
        ranking[i].id_equipe = teams[i].id_equipe;
        strcpy(ranking[i].nome_equipe, teams[i].nome_equipe);
        ranking[i].pontos = calcular_pontuacao(
            teams[i].id_equipe, matches, match_count);
        if (ranking[i].pontos < 0) return 0;
    }

    for (i = 0; i < match_count; i++) {
        int index_a = find_team_index(teams, team_count, matches[i].id_equipeA);
        int index_b = find_team_index(teams, team_count, matches[i].id_equipeB);
        if (index_a < 0 || index_b < 0 || index_a == index_b) return 0;
        ranking[index_a].jogos++;
        ranking[index_b].jogos++;
        ranking[index_a].pontos_pro += matches[i].placarA;
        ranking[index_a].pontos_contra += matches[i].placarB;
        ranking[index_b].pontos_pro += matches[i].placarB;
        ranking[index_b].pontos_contra += matches[i].placarA;
        if (matches[i].placarA > matches[i].placarB) {
            ranking[index_a].vitorias++;
            ranking[index_b].derrotas++;
        } else if (matches[i].placarA < matches[i].placarB) {
            ranking[index_b].vitorias++;
            ranking[index_a].derrotas++;
        } else {
            ranking[index_a].empates++;
            ranking[index_b].empates++;
        }
    }
    return 1;
}

static int official_order_after(const Classificacao *left,
                                const Classificacao *right)
{
    double left_balance = left->pontos_pro - left->pontos_contra;
    double right_balance = right->pontos_pro - right->pontos_contra;
    if (left->pontos != right->pontos) return left->pontos < right->pontos;
    if (left->vitorias != right->vitorias) {
        return left->vitorias < right->vitorias;
    }
    if (left_balance != right_balance) return left_balance < right_balance;
    if (left->pontos_pro != right->pontos_pro) {
        return left->pontos_pro < right->pontos_pro;
    }
    return left->id_equipe > right->id_equipe;
}

static int selected_order_after(const Classificacao *left,
                                const Classificacao *right,
                                const char *sort)
{
    double left_balance = left->pontos_pro - left->pontos_contra;
    double right_balance = right->pontos_pro - right->pontos_contra;

    if (strcmp(sort, "wins") == 0 && left->vitorias != right->vitorias) {
        return left->vitorias < right->vitorias;
    }
    if (strcmp(sort, "balance") == 0 && left_balance != right_balance) {
        return left_balance < right_balance;
    }
    if (strcmp(sort, "id") == 0 && left->id_equipe != right->id_equipe) {
        return left->id_equipe > right->id_equipe;
    }
    return official_order_after(left, right);
}

static void sort_selected(Classificacao ranking[], int count,
                          const char *sort)
{
    int i;
    int j;
    if (strcmp(sort, "classification") == 0 ||
        strcmp(sort, "points") == 0) {
        ordenar_ranking(ranking, count);
        return;
    }
    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (selected_order_after(&ranking[j], &ranking[j + 1], sort)) {
                Classificacao temporary = ranking[j];
                ranking[j] = ranking[j + 1];
                ranking[j + 1] = temporary;
            }
        }
    }
}

static int append_team_rows(Buffer *buffer, const Equipe teams[],
                            int team_count, const char *query)
{
    int i;
    int matches = 0;
    for (i = 0; i < team_count; i++) {
        char id[32];
        snprintf(id, sizeof(id), "%d", teams[i].id_equipe);
        if (!contains_nocase(teams[i].nome_equipe, query) &&
            !contains_nocase(id, query)) {
            continue;
        }
        if (!buffer_appendf(buffer, "<tr><td>%d</td><td>",
                            teams[i].id_equipe) ||
            !html_escape(buffer, teams[i].nome_equipe) ||
            !buffer_appendf(buffer, "</td><td>%d</td></tr>",
                            teams[i].num_jogadores)) {
            return 0;
        }
        matches++;
    }
    if (matches == 0) {
        return buffer_append(buffer,
            "<tr><td colspan=\"3\" class=\"empty\">Nenhuma equipe encontrada.</td></tr>");
    }
    return 1;
}

static int append_player_rows(Buffer *buffer, const Jogador players[],
                             int player_count, const Equipe teams[],
                             int team_count, const char *query)
{
    int i;
    int matches = 0;
    for (i = 0; i < player_count; i++) {
        char id[32];
        int team_index = find_team_index(
            teams, team_count, players[i].id_equipe);
        snprintf(id, sizeof(id), "%d", players[i].id_jogador);
        if (!contains_nocase(players[i].nickname, query) &&
            !contains_nocase(players[i].nome_completo, query) &&
            !contains_nocase(id, query)) {
            continue;
        }
        if (!buffer_appendf(buffer, "<tr><td>%d</td><td>",
                            players[i].id_jogador) ||
            !html_escape(buffer, players[i].nickname) ||
            !buffer_append(buffer, "<br><span class=\"notice-inline\">") ||
            !html_escape(buffer, players[i].nome_completo) ||
            !buffer_append(buffer, "</span></td><td>")) {
            return 0;
        }
        if (team_index < 0) {
            if (!buffer_append(buffer, "Sem equipe")) return 0;
        } else if (!html_escape(buffer, teams[team_index].nome_equipe)) {
            return 0;
        }
        if (!buffer_append(buffer, "</td></tr>")) return 0;
        matches++;
    }
    if (matches == 0) {
        return buffer_append(buffer,
            "<tr><td colspan=\"3\" class=\"empty\">Nenhum jogador encontrado.</td></tr>");
    }
    return 1;
}

static int append_ranking_rows(Buffer *buffer, Classificacao ranking[],
                               int team_count, const char *sort)
{
    int i;
    sort_selected(ranking, team_count, sort);
    for (i = 0; i < team_count; i++) {
        double balance = ranking[i].pontos_pro - ranking[i].pontos_contra;
        if (!buffer_appendf(
                buffer,
                "<tr><td>%dº</td><td><strong>",
                i + 1) ||
            !html_escape(buffer, ranking[i].nome_equipe) ||
            !buffer_appendf(
                buffer,
                "</strong> <span class=\"notice-inline\">(ID %d)</span></td>"
                "<td class=\"center\">%d</td>"
                "<td class=\"center\">%d</td>"
                "<td class=\"center\">%d</td>"
                "<td class=\"center\">%d</td>"
                "<td class=\"right\"><strong>%d</strong></td>"
                "<td class=\"right\">%.2f</td></tr>",
                ranking[i].id_equipe, ranking[i].jogos,
                ranking[i].vitorias, ranking[i].empates,
                ranking[i].derrotas, ranking[i].pontos, balance)) {
            return 0;
        }
    }
    if (team_count == 0) {
        return buffer_append(buffer,
            "<tr><td colspan=\"8\" class=\"empty\">Nenhuma equipe cadastrada.</td></tr>");
    }
    return 1;
}

static int append_matrix(Buffer *head, Buffer *rows,
                         const Equipe teams[], int team_count,
                         const Partida matches[], int match_count)
{
    int i;
    int j;
    if (!buffer_append(head, "<tr><th>Equipe</th>")) return 0;
    for (i = 0; i < team_count; i++) {
        if (!buffer_append(head, "<th>") ||
            !html_escape(head, teams[i].nome_equipe) ||
            !buffer_append(head, "</th>")) {
            return 0;
        }
    }
    if (!buffer_append(head, "</tr>")) return 0;
    for (i = 0; i < team_count; i++) {
        if (!buffer_append(rows, "<tr><th>") ||
            !html_escape(rows, teams[i].nome_equipe) ||
            !buffer_append(rows, "</th>")) {
            return 0;
        }
        for (j = 0; j < team_count; j++) {
            int k;
            int written = 0;
            if (i == j) {
                if (!buffer_append(rows, "<td class=\"center\">—</td>")) {
                    return 0;
                }
                continue;
            }
            if (!buffer_append(rows, "<td>")) return 0;
            for (k = 0; k < match_count; k++) {
                int team_a = teams[i].id_equipe;
                int team_b = teams[j].id_equipe;
                double score_for;
                double score_against;
                if (matches[k].id_equipeA == team_a &&
                    matches[k].id_equipeB == team_b) {
                    score_for = matches[k].placarA;
                    score_against = matches[k].placarB;
                } else if (matches[k].id_equipeA == team_b &&
                           matches[k].id_equipeB == team_a) {
                    score_for = matches[k].placarB;
                    score_against = matches[k].placarA;
                } else {
                    continue;
                }
                if (written && !buffer_append(rows, "<br>")) return 0;
                if (!buffer_appendf(rows, "<span class=\"nowrap\">R%d: %.2f x %.2f</span>",
                                    matches[k].rodada, score_for,
                                    score_against)) {
                    return 0;
                }
                written++;
            }
            if (!written && !buffer_append(rows, "—")) return 0;
            if (!buffer_append(rows, "</td>")) return 0;
        }
        if (!buffer_append(rows, "</tr>")) return 0;
    }
    if (team_count == 0) {
        return buffer_append(rows,
            "<tr><td class=\"empty\">Cadastre equipes para preencher a matriz.</td></tr>");
    }
    return 1;
}

static int append_recent_matches(Buffer *buffer, const Equipe teams[],
                                 int team_count, const Partida matches[],
                                 int match_count)
{
    int i;
    int start = match_count > 5 ? match_count - 5 : 0;
    if (match_count == 0) {
        return buffer_append(buffer,
            "<p class=\"empty\">Nenhuma partida registrada.</p>");
    }
    for (i = match_count - 1; i >= start; i--) {
        int team_a = find_team_index(teams, team_count,
                                     matches[i].id_equipeA);
        int team_b = find_team_index(teams, team_count,
                                     matches[i].id_equipeB);
        if (team_a < 0 || team_b < 0) return 0;
        if (!buffer_append(buffer, "<article class=\"match\"><strong>") ||
            !html_escape(buffer, teams[team_a].nome_equipe) ||
            !buffer_append(buffer, " x ") ||
            !html_escape(buffer, teams[team_b].nome_equipe) ||
            !buffer_appendf(
                buffer,
                "</strong><p><span class=\"badge\">%.2f x %.2f</span>"
                " &nbsp; Rodada %d</p></article>",
                matches[i].placarA, matches[i].placarB, matches[i].rodada)) {
            return 0;
        }
    }
    return 1;
}

static int load_template(Buffer *template)
{
    FILE *file = fopen("visualizacao.html", "rb");
    long length;

    if (file == NULL) {
        fprintf(stderr, "Nao foi possivel abrir visualizacao.html.\n");
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    if ((unsigned long)length > 256UL * 1024UL ||
        !buffer_reserve(template, (size_t)length)) {
        fclose(file);
        return 0;
    }
    if (fread(template->data, 1, (size_t)length, file) != (size_t)length) {
        fclose(file);
        return 0;
    }
    template->length = (size_t)length;
    template->data[template->length] = '\0';
    fclose(file);
    return 1;
}

static int send_all(Socket socket, const char *data, size_t length)
{
    size_t sent = 0;
    while (sent < length) {
        size_t remaining = length - sent;
        int chunk = remaining > 32768U ? 32768 : (int)remaining;
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        int result = send(socket, data + sent, chunk, flags);
        if (result <= 0) return 0;
        sent += (size_t)result;
    }
    return 1;
}

static void send_response(Socket socket, int status, const char *reason,
                          const char *content_type, const char *body,
                          size_t body_length)
{
    char headers[1024];
    int length = snprintf(
        headers, sizeof(headers),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'self'; style-src 'self' 'unsafe-inline'; form-action 'self'; base-uri 'none'\r\n"
        "Connection: close\r\n\r\n",
        status, reason, content_type, (unsigned long)body_length);
    if (length > 0 && (size_t)length < sizeof(headers)) {
        send_all(socket, headers, (size_t)length);
        send_all(socket, body, body_length);
    }
}

static void send_text_error(Socket socket, int status, const char *reason,
                            const char *message)
{
    send_response(socket, status, reason, "text/plain; charset=utf-8",
                  message, strlen(message));
}

static void send_redirect(Socket socket, const char *location)
{
    char response[512];
    int length = snprintf(
        response, sizeof(response),
        "HTTP/1.1 303 See Other\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n\r\n",
        location);
    if (length > 0 && (size_t)length < sizeof(response)) {
        send_all(socket, response, (size_t)length);
    }
}

static int get_content_length(const char *request, const char *header_end,
                              size_t *content_length)
{
    const char *line = strstr(request, "\r\n");
    if (line == NULL) return 0;
    line += 2;
    *content_length = 0;

    while (line < header_end && *line != '\0') {
        const char *line_end = strstr(line, "\r\n");
        const char *colon;
        if (line_end == NULL || line_end > header_end) break;
        colon = memchr(line, ':', (size_t)(line_end - line));
        if (colon != NULL &&
            (size_t)(colon - line) == strlen("Content-Length") &&
            ascii_equal_nocase(line, "Content-Length",
                               strlen("Content-Length"))) {
            char number[32];
            size_t value_length = (size_t)(line_end - colon - 1);
            char *end;
            unsigned long parsed;
            while (value_length > 0 &&
                   (colon[1] == ' ' || colon[1] == '\t')) {
                colon++;
                value_length--;
            }
            if (value_length == 0 || value_length >= sizeof(number)) return 0;
            memcpy(number, colon + 1, value_length);
            number[value_length] = '\0';
            errno = 0;
            parsed = strtoul(number, &end, 10);
            if (errno != 0 || end == number || *end != '\0' ||
                parsed > MAX_REQUEST_BYTES) {
                return 0;
            }
            *content_length = (size_t)parsed;
            return 1;
        }
        line = line_end + 2;
    }
    return 1;
}

static int receive_request(Socket socket, char request[],
                           size_t capacity, size_t *request_length,
                           size_t *header_length, size_t *body_length)
{
    size_t used = 0;
    char *header_end = NULL;
    size_t content_length = 0;
    size_t expected;

    while (header_end == NULL) {
        int received;
        if (used + 1 >= capacity) return 0;
        received = recv(socket, request + used, (int)(capacity - used - 1), 0);
        if (received <= 0) return 0;
        used += (size_t)received;
        request[used] = '\0';
        header_end = strstr(request, "\r\n\r\n");
    }
    if (!get_content_length(request, header_end, &content_length)) return 0;
    *header_length = (size_t)(header_end - request) + 4;
    if (*header_length > capacity - 1 ||
        content_length > capacity - *header_length - 1) {
        return 0;
    }
    expected = *header_length + content_length;
    while (used < expected) {
        int received = recv(socket, request + used,
                            (int)(expected - used), 0);
        if (received <= 0) return 0;
        used += (size_t)received;
    }
    request[used] = '\0';
    *request_length = used;
    *body_length = content_length;
    return 1;
}

static int read_request_line(const char *request, char method[],
                             size_t method_size, char target[],
                             size_t target_size)
{
    char version[32];
    return sscanf(request, "%15s %2047s %31s", method, target, version) == 3 &&
        strlen(method) < method_size && strlen(target) < target_size;
}

static int team_exists(PGconn *connection, const char *table,
                       const char *column, int id)
{
    char sql[160];
    char id_text[32];
    const char *parameters[1];
    PGresult *result;
    int exists;

    if (strcmp(table, "campeonato") != 0 &&
        strcmp(table, "jogos") != 0 &&
        strcmp(table, "equipes") != 0) {
        return 0;
    }
    snprintf(sql, sizeof(sql),
             "SELECT 1 FROM %s WHERE %s = $1 LIMIT 1", table, column);
    snprintf(id_text, sizeof(id_text), "%d", id);
    parameters[0] = id_text;
    result = PQexecParams(connection, sql, 1, NULL, parameters,
                          NULL, NULL, 0);
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Falha ao validar referencia no banco: %s",
                PQerrorMessage(connection));
        if (result != NULL) PQclear(result);
        return -1;
    }
    exists = PQntuples(result) > 0;
    PQclear(result);
    return exists;
}

static void process_post(PGconn *connection, const FormData *form,
                         char message[], size_t message_size,
                         int *status)
{
    const char *action = form_value(form, "action");

    *status = 400;
    if (strcmp(action, "team") == 0) {
        const char *name = form_value(form, "name");
        int id;
        if (!valid_text(name, 100, 1)) {
            snprintf(message, message_size,
                     "Informe um nome de equipe com até 100 caracteres.");
        } else if (!banco_salvar_equipe(connection, name, &id)) {
            snprintf(message, message_size,
                     "Não foi possível cadastrar a equipe. Verifique o banco.");
            *status = 500;
        } else {
            snprintf(message, message_size,
                     "Equipe cadastrada com ID %d.", id);
            *status = 200;
        }
        return;
    }

    if (strcmp(action, "player") == 0) {
        int cpf;
        int team_id;
        const char *full_name = form_value(form, "full_name");
        const char *nickname = form_value(form, "nickname");
        const char *steam = form_value(form, "steam");
        const char *phone = form_value(form, "phone");
        if (!parse_positive_int(form_value(form, "cpf"), &cpf) ||
            !valid_text(full_name, 100, 1) ||
            !valid_text(nickname, 50, 1) ||
            !valid_text(steam, 100, 0) ||
            !valid_phone(phone) ||
            !parse_optional_id(form_value(form, "team_id"), &team_id)) {
            snprintf(message, message_size,
                     "Revise os dados do jogador e os limites dos campos.");
            return;
        }
        if (team_id != SEM_EQUIPE) {
            int exists = team_exists(connection, "equipes", "id_equipe",
                                     team_id);
            if (exists < 0) {
                snprintf(message, message_size,
                         "Falha ao validar a equipe no banco.");
                *status = 500;
                return;
            }
            if (!exists) {
                snprintf(message, message_size,
                         "A equipe selecionada não existe.");
                return;
            }
        }
        if (!banco_salvar_jogador(connection, cpf, full_name, nickname,
                                  steam, phone, team_id)) {
            snprintf(message, message_size,
                     "Não foi possível cadastrar o jogador. Verifique CPF e banco.");
            *status = 500;
            return;
        }
        snprintf(message, message_size, "Jogador cadastrado com sucesso.");
        *status = 200;
        return;
    }

    if (strcmp(action, "match") == 0) {
        Partida match;
        memset(&match, 0, sizeof(match));
        if (!parse_positive_int(form_value(form, "round"), &match.rodada) ||
            !parse_positive_int(form_value(form, "championship_id"),
                                &match.id_campeonato) ||
            !parse_positive_int(form_value(form, "game_id"), &match.id_jogo) ||
            !parse_positive_int(form_value(form, "team_a"),
                                &match.id_equipeA) ||
            !parse_positive_int(form_value(form, "team_b"),
                                &match.id_equipeB) ||
            !parse_nonnegative_decimal(form_value(form, "score_a"),
                                       &match.placarA) ||
            !parse_nonnegative_decimal(form_value(form, "score_b"),
                                       &match.placarB)) {
            snprintf(message, message_size,
                     "Revise rodada, equipes, campeonato, jogo e placares.");
            return;
        }
        if (match.id_equipeA == match.id_equipeB) {
            snprintf(message, message_size,
                     "Uma equipe não pode jogar contra ela mesma.");
            return;
        }
        {
            int championship_exists = team_exists(
                connection, "campeonato", "id_campeonato",
                match.id_campeonato);
            int game_exists = team_exists(
                connection, "jogos", "id_jogo", match.id_jogo);
            int team_a_exists = team_exists(
                connection, "equipes", "id_equipe", match.id_equipeA);
            int team_b_exists = team_exists(
                connection, "equipes", "id_equipe", match.id_equipeB);
            if (championship_exists < 0 || game_exists < 0 ||
                team_a_exists < 0 || team_b_exists < 0) {
                snprintf(message, message_size,
                         "Falha ao validar referências no banco.");
                *status = 500;
                return;
            }
            if (!championship_exists || !game_exists ||
                !team_a_exists || !team_b_exists) {
                snprintf(message, message_size,
                         "Selecione registros existentes de campeonato, jogo e equipes.");
                return;
            }
        }
        if (!banco_salvar_partida(connection, &match)) {
            snprintf(message, message_size,
                     "Não foi possível registrar a partida. Verifique o banco.");
            *status = 500;
            return;
        }
        snprintf(message, message_size,
                 "Partida %d e os dois resultados foram registrados.",
                 match.id_partida);
        *status = 200;
        return;
    }
    snprintf(message, message_size, "Ação de formulário desconhecida.");
}

static int render_dashboard(PGconn *connection, const FormData *query,
                            const char *notice, Buffer *page)
{
    Equipe teams[MAX_EQUIPES];
    Jogador players[MAX_JOGADORES];
    Partida matches[MAX_PARTIDAS];
    Classificacao ranking[MAX_EQUIPES];
    int team_count = 0;
    int player_count = 0;
    int match_count = 0;
    int latest_round = 0;
    int i;
    Buffer team_options;
    Buffer championship_options;
    Buffer game_options;
    Buffer team_rows;
    Buffer player_rows;
    Buffer ranking_rows;
    Buffer matrix_head;
    Buffer matrix_rows;
    Buffer recent_matches;
    Buffer escaped_notice;
    Buffer template;
    Buffer notice_html;
    char team_count_text[32];
    char match_count_text[32];
    char latest_round_text[32];
    const char *team_search = form_value(query, "qteam");
    const char *player_search = form_value(query, "qplayer");
    const char *sort = form_value(query, "sort");
    char selected_classification[16] = "";
    char selected_points[16] = "";
    char selected_wins[16] = "";
    char selected_balance[16] = "";
    char selected_id[16] = "";
    TemplateValue values[20];
    size_t value_count = 0;
    int ok = 0;

    buffer_init(&team_options);
    buffer_init(&championship_options);
    buffer_init(&game_options);
    buffer_init(&team_rows);
    buffer_init(&player_rows);
    buffer_init(&ranking_rows);
    buffer_init(&matrix_head);
    buffer_init(&matrix_rows);
    buffer_init(&recent_matches);
    buffer_init(&escaped_notice);
    buffer_init(&template);
    buffer_init(&notice_html);

    if (sort[0] == '\0' ||
        (strcmp(sort, "classification") != 0 &&
         strcmp(sort, "points") != 0 &&
         strcmp(sort, "wins") != 0 &&
         strcmp(sort, "balance") != 0 &&
         strcmp(sort, "id") != 0)) {
        sort = "classification";
    }
    if (!banco_carregar_equipes(connection, teams, MAX_EQUIPES, &team_count) ||
        !banco_carregar_jogadores(connection, players, MAX_JOGADORES,
                                  &player_count) ||
        !banco_carregar_partidas(connection, matches, MAX_PARTIDAS,
                                 &match_count)) {
        goto cleanup;
    }
    ordenar_equipes_por_id(teams, team_count);
    if (!build_classification(teams, team_count, matches, match_count,
                              ranking)) {
        fprintf(stderr, "Falha ao montar a classificacao para a pagina.\n");
        goto cleanup;
    }
    for (i = 0; i < match_count; i++) {
        if (matches[i].rodada > latest_round) latest_round = matches[i].rodada;
    }
    snprintf(team_count_text, sizeof(team_count_text), "%d", team_count);
    snprintf(match_count_text, sizeof(match_count_text), "%d", match_count);
    snprintf(latest_round_text, sizeof(latest_round_text), "%d", latest_round);

    if (!append_team_options(&team_options, teams, team_count) ||
        !append_options(&championship_options, connection,
            "SELECT id_campeonato, COALESCE(nome_campeonato, '') "
            "FROM campeonato ORDER BY id_campeonato", 0, 1) ||
        !append_options(&game_options, connection,
            "SELECT id_jogo, COALESCE(nome, '') FROM jogos ORDER BY id_jogo",
            0, 1) ||
        !append_team_rows(&team_rows, teams, team_count, team_search) ||
        !append_player_rows(&player_rows, players, player_count, teams,
                            team_count, player_search) ||
        !append_ranking_rows(&ranking_rows, ranking, team_count, sort) ||
        !append_matrix(&matrix_head, &matrix_rows, teams, team_count,
                       matches, match_count) ||
        !append_recent_matches(&recent_matches, teams, team_count,
                               matches, match_count) ||
        !html_escape(&escaped_notice, notice) ||
        !load_template(&template)) {
        goto cleanup;
    }

    if (notice[0] != '\0' &&
        (!buffer_append(&notice_html, "<div class=\"notice\" role=\"status\">") ||
         !buffer_append_n(&notice_html, escaped_notice.data,
                          escaped_notice.length) ||
         !buffer_append(&notice_html, "</div>"))) {
        goto cleanup;
    }
    if (strcmp(sort, "classification") == 0) {
        strcpy(selected_classification, "selected");
    } else if (strcmp(sort, "points") == 0) {
        strcpy(selected_points, "selected");
    } else if (strcmp(sort, "wins") == 0) {
        strcpy(selected_wins, "selected");
    } else if (strcmp(sort, "balance") == 0) {
        strcpy(selected_balance, "selected");
    } else if (strcmp(sort, "id") == 0) {
        strcpy(selected_id, "selected");
    }

    values[value_count++] = (TemplateValue){"{{NOTICE}}", notice_html.data};
    values[value_count++] = (TemplateValue){"{{TEAM_COUNT}}", team_count_text};
    values[value_count++] = (TemplateValue){"{{MATCH_COUNT}}", match_count_text};
    values[value_count++] = (TemplateValue){"{{LATEST_ROUND}}", latest_round_text};
    values[value_count++] = (TemplateValue){"{{TEAM_OPTIONS}}", team_options.data};
    values[value_count++] = (TemplateValue){"{{CHAMPIONSHIP_OPTIONS}}", championship_options.data};
    values[value_count++] = (TemplateValue){"{{GAME_OPTIONS}}", game_options.data};
    values[value_count++] = (TemplateValue){"{{TEAM_ROWS}}", team_rows.data};
    values[value_count++] = (TemplateValue){"{{PLAYER_ROWS}}", player_rows.data};
    values[value_count++] = (TemplateValue){"{{RANKING_ROWS}}", ranking_rows.data};
    values[value_count++] = (TemplateValue){"{{MATRIX_HEAD}}", matrix_head.data};
    values[value_count++] = (TemplateValue){"{{MATRIX_ROWS}}", matrix_rows.data};
    values[value_count++] = (TemplateValue){"{{MATCH_ROWS}}", recent_matches.data};
    values[value_count++] = (TemplateValue){"{{SORT_CLASSIFICATION}}", selected_classification};
    values[value_count++] = (TemplateValue){"{{SORT_POINTS}}", selected_points};
    values[value_count++] = (TemplateValue){"{{SORT_WINS}}", selected_wins};
    values[value_count++] = (TemplateValue){"{{SORT_BALANCE}}", selected_balance};
    values[value_count++] = (TemplateValue){"{{SORT_ID}}", selected_id};

    {
        Buffer escaped_team_search;
        Buffer escaped_player_search;
        Buffer final_page;
        buffer_init(&escaped_team_search);
        buffer_init(&escaped_player_search);
        buffer_init(&final_page);
        if (!html_escape(&escaped_team_search, team_search) ||
            !html_escape(&escaped_player_search, player_search)) {
            buffer_free(&escaped_team_search);
            buffer_free(&escaped_player_search);
            buffer_free(&final_page);
            goto cleanup;
        }
        values[value_count++] = (TemplateValue){"{{TEAM_SEARCH}}",
                                                escaped_team_search.data};
        values[value_count++] = (TemplateValue){"{{PLAYER_SEARCH}}",
                                                escaped_player_search.data};
        if (!replace_template(template.data, values, value_count, &final_page)) {
            buffer_free(&escaped_team_search);
            buffer_free(&escaped_player_search);
            buffer_free(&final_page);
            goto cleanup;
        }
        *page = final_page;
        buffer_free(&escaped_team_search);
        buffer_free(&escaped_player_search);
    }
    if (page->failed || template.failed || team_options.failed ||
        championship_options.failed || game_options.failed ||
        team_rows.failed || player_rows.failed || ranking_rows.failed ||
        matrix_head.failed || matrix_rows.failed || recent_matches.failed) {
        goto cleanup;
    }
    ok = 1;

cleanup:
    buffer_free(&team_options);
    buffer_free(&championship_options);
    buffer_free(&game_options);
    buffer_free(&team_rows);
    buffer_free(&player_rows);
    buffer_free(&ranking_rows);
    buffer_free(&matrix_head);
    buffer_free(&matrix_rows);
    buffer_free(&recent_matches);
    buffer_free(&escaped_notice);
    buffer_free(&template);
    buffer_free(&notice_html);
    return ok;
}

static int parse_target_query(const char *target, FormData *query)
{
    const char *question = strchr(target, '?');
    if (question == NULL) {
        memset(query, 0, sizeof(*query));
        return 1;
    }
    return parse_form_data(question + 1, strlen(question + 1), query);
}

static void handle_client(Socket client)
{
    char request[MAX_REQUEST_BYTES + 1];
    size_t request_length;
    size_t header_length;
    size_t body_length;
    char method[16];
    char target[2048];
    FormData form;
    PGconn *connection = NULL;
    char notice[256] = "";
    const char *saved_action;
    int status = 200;
    Buffer page;

    if (!receive_request(client, request, sizeof(request), &request_length,
                         &header_length, &body_length) ||
        !read_request_line(request, method, sizeof(method),
                           target, sizeof(target))) {
        send_text_error(client, 400, "Bad Request", "Requisicao HTTP invalida.");
        return;
    }
    (void)request_length;

    if (strcmp(target, "/") != 0 &&
        strncmp(target, "/?", 2) != 0 &&
        strcmp(target, "/visualizacao.html") != 0 &&
        strncmp(target, "/visualizacao.html?", 19) != 0) {
        send_text_error(client, 404, "Not Found", "Pagina nao encontrada.");
        return;
    }
    if (strcmp(method, "GET") == 0) {
        if (!parse_target_query(target, &form)) {
            send_text_error(client, 400, "Bad Request", "Pesquisa invalida.");
            return;
        }
    } else if (strcmp(method, "POST") == 0) {
        if (!has_form_content_type(request) ||
            !parse_form_data(request + header_length, body_length, &form)) {
            send_text_error(client, 400, "Bad Request",
                            "Formulario invalido ou incompleto.");
            return;
        }
    } else {
        send_text_error(client, 405, "Method Not Allowed",
                        "Use GET ou POST para acessar esta pagina.");
        return;
    }

    if (!banco_conectar(&connection)) {
        send_text_error(client, 503, "Service Unavailable",
                        "Nao foi possivel conectar ao PostgreSQL. "
                        "Configure as variaveis PG* e PGPASSWORD.");
        return;
    }
    if (strcmp(method, "POST") == 0) {
        process_post(connection, &form, notice, sizeof(notice), &status);
        if (status == 200) {
            const char *action = form_value(&form, "action");
            if (strcmp(action, "team") == 0) {
                send_redirect(client, "/?saved=team");
            } else if (strcmp(action, "player") == 0) {
                send_redirect(client, "/?saved=player");
            } else {
                send_redirect(client, "/?saved=match");
            }
            banco_desconectar(connection);
            return;
        }
    } else {
        saved_action = form_value(&form, "saved");
        if (strcmp(saved_action, "team") == 0) {
            snprintf(notice, sizeof(notice), "Equipe cadastrada com sucesso.");
        } else if (strcmp(saved_action, "player") == 0) {
            snprintf(notice, sizeof(notice), "Jogador cadastrado com sucesso.");
        } else if (strcmp(saved_action, "match") == 0) {
            snprintf(notice, sizeof(notice),
                     "Partida e resultados registrados com sucesso.");
        }
    }

    buffer_init(&page);
    if (!render_dashboard(connection, &form, notice, &page)) {
        send_text_error(client, 500, "Internal Server Error",
                        "Nao foi possivel montar a pagina. Verifique o log do servidor.");
    } else {
        send_response(client, status,
                      status == 200 ? "OK" :
                      (status == 500 ? "Internal Server Error" : "Bad Request"),
                      "text/html; charset=utf-8", page.data, page.length);
    }
    buffer_free(&page);
    banco_desconectar(connection);
}

static int parse_port(const char *text, char port_text[], size_t capacity)
{
    char *end;
    long port;
    if (text == NULL || text[0] == '\0') text = "3000";
    errno = 0;
    port = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        port < 1 || port > 65535) {
        return 0;
    }
    snprintf(port_text, capacity, "%ld", port);
    return 1;
}

int main(void)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    Socket listener = INVALID_SOCKET_VALUE;
    char port_text[16];
#ifdef _WIN32
    WSADATA winsock_data;
    if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0) {
        fprintf(stderr, "Nao foi possivel iniciar o Winsock.\n");
        return 1;
    }
#endif

    if (!parse_port(getenv("PORT"), port_text, sizeof(port_text))) {
        fprintf(stderr, "PORT deve estar entre 1 e 65535.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo("127.0.0.1", port_text, &hints, &addresses) != 0) {
        fprintf(stderr, "Nao foi possivel configurar a porta HTTP.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        int reuse = 1;
        listener = socket(address->ai_family, address->ai_socktype,
                          address->ai_protocol);
        if (listener == INVALID_SOCKET_VALUE) continue;
#ifdef _WIN32
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&reuse, sizeof(reuse));
#else
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse));
#endif
        if (bind(listener, address->ai_addr,
                 (int)address->ai_addrlen) == 0 &&
            listen(listener, 8) == 0) {
            break;
        }
        CLOSE_SOCKET(listener);
        listener = INVALID_SOCKET_VALUE;
    }
    freeaddrinfo(addresses);
    if (listener == INVALID_SOCKET_VALUE) {
        fprintf(stderr, "Nao foi possivel abrir a porta %s.\n", port_text);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("Aplicacao em C disponivel em http://127.0.0.1:%s\n", port_text);
    printf("Pressione Ctrl+C para encerrar.\n");
    for (;;) {
        Socket client = accept(listener, NULL, NULL);
        if (client == INVALID_SOCKET_VALUE) {
            fprintf(stderr, "Falha ao aceitar conexao HTTP.\n");
            continue;
        }
        handle_client(client);
        CLOSE_SOCKET(client);
    }
}
