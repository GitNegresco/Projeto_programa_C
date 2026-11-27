-- Campeonato de E-Sports  |  schema.sql (FINAL)  |  PostgreSQL 14+
--
-- v4: contrato de persistencia/banco.h + tabela PONTUACAO (geral por equipe)
--   - jogador tem id gerado (id_jogador);
--   - jogador pode existir sem equipe (id_equipe NULL)
--   - partida usa campeonato/jogo PADRAO (id 1) quando nao informados
--   - pontuacao: 1 linha por equipe, mantida por trigger (nao precisa
--     chamar nada pelo C)
-- =====================================================================

DROP VIEW     IF EXISTS classificacao;
DROP TABLE    IF EXISTS pontuacao  CASCADE;
DROP TABLE    IF EXISTS resultados CASCADE;
DROP TABLE    IF EXISTS partidas   CASCADE;
DROP TABLE    IF EXISTS participam CASCADE;
DROP TABLE    IF EXISTS jogadores  CASCADE;
DROP TABLE    IF EXISTS campeonato CASCADE;
DROP TABLE    IF EXISTS jogos      CASCADE;
DROP TABLE    IF EXISTS equipes    CASCADE;
DROP FUNCTION IF EXISTS atualizar_pontuacao();

-- ---------------------------------------------------------------------
-- Tabelas independentes
-- ---------------------------------------------------------------------
CREATE TABLE equipes (
    id_equipe    INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome_equipe  VARCHAR(100) NOT NULL UNIQUE
);

CREATE TABLE jogos (
    id_jogo         INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome            VARCHAR(100) NOT NULL,
    desenvolvedora  VARCHAR(100),
    tipo_jogo       VARCHAR(50)
);

CREATE TABLE campeonato (
    id_campeonato    INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nome_campeonato  VARCHAR(50)  NOT NULL,
    formato          VARCHAR(100) NOT NULL
);

-- Linhas padrao: a partida aponta para elas quando o C nao informa campeonato/jogo
INSERT INTO campeonato (nome_campeonato, formato) VALUES ('Campeonato padrao', 'pontos corridos');
INSERT INTO jogos (nome) VALUES ('Jogo padrao');

-- ---------------------------------------------------------------------
-- Tabelas com FK
-- ---------------------------------------------------------------------
CREATE TABLE jogadores (
    id_jogador     INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    nickname       VARCHAR(50) NOT NULL,
    cpf            CHAR(11) UNIQUE
                   CHECK (cpf ~ '^[0-9]{11}$'),      -- opcional; se vier, 11 digitos
    nome_completo  VARCHAR(100),
    conta_steam    VARCHAR(50),
    telefone       VARCHAR(15),
    id_equipe      INT                               -- NULL = jogador sem equipe
                   REFERENCES equipes (id_equipe) ON DELETE SET NULL
);

-- Participam: N:N entre campeonato e equipes (inscricao)
CREATE TABLE participam (
    id_campeonato  INT NOT NULL
                   REFERENCES campeonato (id_campeonato) ON DELETE RESTRICT,
    id_equipe      INT NOT NULL
                   REFERENCES equipes (id_equipe) ON DELETE CASCADE,
    PRIMARY KEY (id_campeonato, id_equipe)
);

CREATE TABLE partidas (
    id_partida     INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    num_rodada     INT NOT NULL CHECK (num_rodada > 0),
    id_campeonato  INT NOT NULL DEFAULT 1
                   REFERENCES campeonato (id_campeonato) ON DELETE RESTRICT,
    id_jogo        INT NOT NULL DEFAULT 1
                   REFERENCES jogos (id_jogo) ON DELETE RESTRICT
);

-- 1 linha por EQUIPE em cada PARTIDA (2 linhas por partida).
-- A linha de menor id_resultado e' o "lado A"; a de maior, o "lado B".
CREATE TABLE resultados (
    id_resultado       INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    id_partida         INT NOT NULL
                       REFERENCES partidas (id_partida) ON DELETE CASCADE,
    id_equipe          INT NOT NULL
                       REFERENCES equipes (id_equipe) ON DELETE RESTRICT,
    resultado_partida  INT NOT NULL CHECK (resultado_partida >= 0),  -- placar da equipe
    kills              INT NOT NULL DEFAULT 0 CHECK (kills  >= 0),
    mortes             INT NOT NULL DEFAULT 0 CHECK (mortes >= 0),
    dano               NUMERIC(10,2) NOT NULL DEFAULT 0 CHECK (dano >= 0),
    cura_realizada     NUMERIC(10,2) NOT NULL DEFAULT 0 CHECK (cura_realizada >= 0),
    UNIQUE (id_partida, id_equipe)
);

-- ---------------------------------------------------------------------
-- PONTUACAO: totais gerais de cada equipe (todos os campeonatos).
-- 1 linha por equipe (UNIQUE). Esses dados são DERIVADO de resultados: quem
-- mantem atualizado é o trigger abaixo. Nao escrever nela a mao.
-- Pontos: vitoria = 3, empate = 1, derrota = 0 (premissa nossa).
-- ---------------------------------------------------------------------
CREATE TABLE pontuacao (
    id_pontuacao  INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    id_equipe     INT NOT NULL UNIQUE
                  REFERENCES equipes (id_equipe) ON DELETE CASCADE,
    n_vitoria     INT NOT NULL DEFAULT 0,
    n_derrota     INT NOT NULL DEFAULT 0,
    n_empate      INT NOT NULL DEFAULT 0,
    n_kills       INT NOT NULL DEFAULT 0,
    n_mortes      INT NOT NULL DEFAULT 0,
    dano_total    NUMERIC(12,2) NOT NULL DEFAULT 0,
    cura_total    NUMERIC(12,2) NOT NULL DEFAULT 0,
    n_pontos      INT NOT NULL DEFAULT 0
);

CREATE FUNCTION atualizar_pontuacao() RETURNS trigger
LANGUAGE plpgsql AS $$
BEGIN
    -- Recalcula tudo a partir de resultados e grava 1 linha por equipe
    INSERT INTO pontuacao (id_equipe, n_vitoria, n_derrota, n_empate,
                           n_kills, n_mortes, dano_total, cura_total, n_pontos)
    SELECT  e.id_equipe,
            COALESCE(c.vitorias, 0), COALESCE(c.derrotas, 0), COALESCE(c.empates, 0),
            COALESCE(c.kills, 0),    COALESCE(c.mortes, 0),
            COALESCE(c.dano, 0),     COALESCE(c.cura, 0),
            3 * COALESCE(c.vitorias, 0) + COALESCE(c.empates, 0)
    FROM    equipes e
    LEFT JOIN (
        SELECT  r.id_equipe,
                COUNT(*) FILTER (WHERE r.resultado_partida >  o.resultado_partida) AS vitorias,
                COUNT(*) FILTER (WHERE r.resultado_partida <  o.resultado_partida) AS derrotas,
                COUNT(*) FILTER (WHERE r.resultado_partida =  o.resultado_partida) AS empates,
                SUM(r.kills) AS kills, SUM(r.mortes) AS mortes,
                SUM(r.dano)  AS dano,  SUM(r.cura_realizada) AS cura
        FROM    resultados r
        JOIN    resultados o ON o.id_partida = r.id_partida
                            AND o.id_equipe <> r.id_equipe
        GROUP BY r.id_equipe
    ) c ON c.id_equipe = e.id_equipe
    ON CONFLICT (id_equipe) DO UPDATE SET
        n_vitoria  = EXCLUDED.n_vitoria,  n_derrota = EXCLUDED.n_derrota,
        n_empate   = EXCLUDED.n_empate,   n_kills   = EXCLUDED.n_kills,
        n_mortes   = EXCLUDED.n_mortes,   dano_total = EXCLUDED.dano_total,
        cura_total = EXCLUDED.cura_total, n_pontos  = EXCLUDED.n_pontos;
    RETURN NULL;
END;
$$;

-- Equipe nova ganha sua linha zerada; qualquer mudanca em resultados recalcula
CREATE TRIGGER trg_pontuacao_equipes
    AFTER INSERT ON equipes
    FOR EACH STATEMENT EXECUTE FUNCTION atualizar_pontuacao();

CREATE TRIGGER trg_pontuacao_resultados
    AFTER INSERT OR UPDATE OR DELETE ON resultados
    FOR EACH STATEMENT EXECUTE FUNCTION atualizar_pontuacao();

-- ---------------------------------------------------------------------
-- Classificacao POR CAMPEONATO (calculada na hora, sem armazenar).
-- Sem ORDER BY de proposito: a ordenacao do ranking é feita em C.
-- ---------------------------------------------------------------------
CREATE VIEW classificacao AS
SELECT  p.id_campeonato,
        r.id_equipe,
        COUNT(*)                                                           AS jogos,
        COUNT(*) FILTER (WHERE r.resultado_partida >  o.resultado_partida) AS vitorias,
        COUNT(*) FILTER (WHERE r.resultado_partida =  o.resultado_partida) AS empates,
        COUNT(*) FILTER (WHERE r.resultado_partida <  o.resultado_partida) AS derrotas,
        3 * COUNT(*) FILTER (WHERE r.resultado_partida >  o.resultado_partida)
          +     COUNT(*) FILTER (WHERE r.resultado_partida =  o.resultado_partida) AS pontos,
        SUM(r.kills)           AS kills,
        SUM(r.mortes)          AS mortes,
        SUM(r.dano)            AS dano,
        SUM(r.cura_realizada)  AS cura_realizada
FROM    resultados r
JOIN    partidas   p ON p.id_partida = r.id_partida
JOIN    resultados o ON o.id_partida = r.id_partida
                    AND o.id_equipe <> r.id_equipe
GROUP BY p.id_campeonato, r.id_equipe;
