Projeto de Campeonato E-Sports

Integrantes:
- Christian Otto Ricarth (10253902)
- Felipe Negreiros Souza (10771620)
- João Manoel Pessoa Carneiro (10771808)
- Lucas Prado Fernandes Cardozo (10771939)

Descrição e principais funcionalidades:

Organizar e definir os jogos, equipes, jogadores de cada equipe, pontuações, partidas e pontos do campeonato. Vai poder visualizar as tabelas de pontuação, equipes e a estrutura do campeonato como um todo, cadastrar os jogadores nas equipes, definir qual jogo e qual partida será realizada e montar as partidas pra um campeonato, o tipo de campeonato foi escolhido por pontos corridos (mais eficientes para aplicar matrizes e buscas).

Algoritmos implementados:

Linguagem C (projeto todo);
Structs e arrays(vetores) (para cadastro de jogadores, formação de partidas, definição de jogo, preenchimento de critérios);
Validações (para todos os dados inseridos nos campos corretos ou não);
Buscas linear, binária (para selecionar e puxar os dados pedidos);  
Matriz ou estrutura parecida (cálculo de pontuação do time talvez);
Ordenação bubble (para percorrer vetores que armazenem dados ou estatísticas do jogador e organiza as tabelas);
Ir colocando aqui quando for fazendo o projeto...

Banco de dados e bilbioteca/driver escolhido:

Banco criado usando PostgreSQL, e a biblioteca libpq.

Justificativa pra escolha desse banco ou biblioteca:

O PostgreSQL pois uma maior facilidade em fazer conexões e integrações para outras plataformas, algo que permite uma futuro aprimoramento do projeto a longo prazo se for desejado, algo que o MYSQL não proporcionaria de igual forma.

Estrutura de tabelas:

Entidades (Jogador, Equipe, Partida, Criterios_pontuacao, Jogo, Campenato);
jogador (CPF_id, nome completo, apelido, conta steam, telefone, funcao);
Equipe (id_equipe, nome, FK CPF_jogador, numero_jogadores, pontuacao);
Partida (FK id_equipes, id_partida, num_rodada, id_jogo);
Jogo (id_jogo, nome, desenvolvedora, tipo_jogo);
Criterios_pontuacao (vitoria, derrota, empate, num_kills, num_mortes, dano_cura_realizados);
Campeonato (nome, quant_partidas, quant_equipes, formato, FK id_partidas, FK id_equipes);

Instruções de Configuração, Compilação, e Execução:

As instruções de compilação e execução estão na seção "Parte do Christian: ranking, busca, menu e persistência".

Referências utilizadas na pesquisa:

Próprio Curso de Algoritmos e Programação 2, Curso de Banco de dados, Documentação do PostgreSQL e ......

## Parte do Christian: ranking, busca, menu e persistência

O programa carrega equipes e partidas do PostgreSQL ao iniciar. Cadastros de equipes e resultados de partidas são gravados no banco; a classificação é calculada a partir dos resultados carregados.

- A busca binária procura o ID da equipe em um vetor carregado em ordem crescente.
- O ranking é organizado por Bubble Sort: pontos, vitórias, saldo, pontos pró e ID.
- Vitória vale 3 pontos, empate vale 1 e derrota vale 0.
- Para registrar uma partida, o menu pede o ID de um campeonato e de um jogo já cadastrados.
- Cada partida grava uma linha em `Partidas` e duas linhas em `Resultados`, uma por equipe.

### Preparar o banco

1. Crie um banco PostgreSQL vazio.
2. Execute uma vez o script `persistencia/schema.sql`.
3. Configure a variável de ambiente `DATABASE_URL` com a conexão do seu banco. Não coloque a senha no código nem a envie no repositório.
4. Cadastre equipes pelo programa. Antes de registrar partidas, cadastre ao menos um jogo e um campeonato nas tabelas `jogos` e `campeonato`. O campeonato precisa referenciar uma equipe existente, conforme o esquema fornecido.

Exemplo de dados iniciais (depois de cadastrar uma equipe de ID `1` pelo programa):

```sql
INSERT INTO jogos (id_jogo, nome, desenvolvedora, tipo_jogo)
VALUES (1, 'League of Legends', 'Riot Games', 'MOBA');

INSERT INTO campeonato (id_campeonato, nome_campeonato, formato, fk_equipes_id_equipe)
VALUES (1, 'Campeonato e-Sports', 'Pontos corridos', 1);
```

Exemplo de configuração no PowerShell (substitua pelos dados locais do seu banco):

```powershell
$env:DATABASE_URL = "postgresql://usuario:senha@localhost:5432/nome_do_banco"
```

### Compilar e executar

É necessário ter o GCC e os arquivos de desenvolvimento do libpq instalados e disponíveis nos caminhos de include e biblioteca do compilador.

Linux/macOS:

```text
gcc -std=c99 -Wall -Wextra -pedantic main.c persistencia/banco.c -lpq -o campeonato
./campeonato
```

Windows com MinGW:

```text
gcc -std=c99 -Wall -Wextra -pedantic main.c persistencia\banco.c -lpq -o campeonato.exe
.\campeonato.exe
```

### Regras de dados

- O programa trabalha com até 100 equipes e 1000 partidas por execução.
- Nomes digitados no menu não podem conter espaços.
- Os placares são armazenados em `Resultados.resultado_partida` e devem ser inteiros.
- Cada partida precisa ter exatamente dois resultados para ser carregada no ranking.
- O código usa `stdio.h`, libpq e o cabeçalho local `persistencia/banco.h`.

### Prévia visual

Abra [visualizacao.html](visualizacao.html) em um navegador para ver a proposta de interface. A página é uma prévia estática e usa dados ilustrativos; não está conectada ao programa C.
