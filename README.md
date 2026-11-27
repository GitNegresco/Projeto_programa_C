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

## Menu, ranking, busca e persistência

O programa carrega equipes, jogadores e partidas do PostgreSQL ao iniciar. As partidas são reconstruídas a partir dos registros de `Partidas` e dos dois resultados correspondentes em `Resultados`. O cadastro pelo menu grava a equipe, ou a partida e seus dois resultados, em uma transação.

- A busca binária procura o ID da equipe em um vetor ordenado por ID.
- O ranking é calculado a partir dos placares em `Resultados`: vitória vale 3 pontos, empate vale 1 e derrota vale 0.
- O Bubble Sort organiza o ranking por pontos, vitórias, saldo, pontos pró e ID da equipe.
- Para registrar uma partida, selecione um campeonato e um jogo existentes, duas equipes diferentes, os placares e a rodada.
- A quantidade de jogadores exibida por equipe é obtida contando as linhas relacionadas em `Jogadores`.
- A página `visualizacao.html` carrega o painel pela API web, sem expor credenciais do PostgreSQL no navegador.

### Preparar o banco

1. Cada integrante instala o PostgreSQL e cria, pelo pgAdmin, seu próprio banco chamado `Campeonato_Esports`.
2. Execute uma vez nesse banco o script `persistencia/schema.sql`. O banco fica na máquina de cada integrante; somente o script de esquema é compartilhado pelo Git.
3. Configure `PGHOST`, `PGPORT`, `PGDATABASE`, `PGUSER` e `PGPASSWORD` no ambiente local. Use `PGDATABASE=Campeonato_Esports`. A senha deve ficar somente em `PGPASSWORD`, nunca no código nem no Git. Para a aplicação web, copie `.env.example` para `.env` e preencha os valores locais; `.env` está ignorado pelo Git.
4. Cadastre as equipes pelo programa. Antes de registrar partidas, cadastre ao menos um jogo e um campeonato nas tabelas `jogos` e `campeonato`. O campeonato precisa referenciar uma equipe existente, conforme o esquema fornecido.

Exemplo de dados iniciais (depois de cadastrar uma equipe de ID `1` pelo programa):

```sql
INSERT INTO jogos (id_jogo, nome, desenvolvedora, tipo_jogo)
VALUES (1, 'League of Legends', 'Riot Games', 'MOBA');

INSERT INTO campeonato (id_campeonato, nome_campeonato, formato, fk_equipes_id_equipe)
VALUES (1, 'Campeonato e-Sports', 'Pontos corridos', 1);
```

Decisões do modelo:

- Jogador sem equipe deve ter `fk_Equipes_id_equipe = NULL`; não grave `-1` no banco.
- Em cada partida, o resultado com menor `id_resultado` representa a equipe A; o seguinte representa a equipe B.
- `Pontuacao` é atualizada pelo trigger criado em `schema.sql`; não atualize essa tabela manualmente.

### Compilar e executar

É necessário ter o GCC e os arquivos de desenvolvimento do libpq instalados e disponíveis nos caminhos de include e biblioteca do compilador.

Para conectar o programa C, configure as variáveis libpq no terminal. Exemplo no PowerShell (defina a senha localmente):

```powershell
$env:PGHOST = "localhost"
$env:PGPORT = "5432"
$env:PGDATABASE = "Campeonato_Esports"
$env:PGUSER = "postgres"
$env:PGPASSWORD = "defina-localmente"
```

Linux/macOS:

```text
gcc -std=c99 -Wall -Wextra -pedantic main.c algoritmos/busca.c algoritmos/ordenacao.c modelos/entidade.c persistencia/banco.c -lpq -lm -o campeonato
./campeonato
```

Windows com MinGW:

```text
gcc -std=c99 -Wall -Wextra -pedantic main.c algoritmos\busca.c algoritmos\ordenacao.c modelos\entidade.c persistencia\banco.c -lpq -lm -o campeonato.exe
.\campeonato.exe
```

### Executar a versão web

A versão web usa Node.js 18 ou superior e não precisa do compilador C nem do libpq. O servidor Node atende a página e a rota `GET /api/dashboard`; a conexão com PostgreSQL é feita somente no servidor, usando `PGPASSWORD` e as demais variáveis `PG*` do arquivo `.env`. Também é aceita `DATABASE_URL` em ambientes hospedados.

Para preparar uma vez em cada computador, com Node.js e PostgreSQL instalados:

```powershell
npm install
Copy-Item .env.example .env
# Edite .env com PGUSER e PGPASSWORD locais. Não compartilhe nem faça commit desse arquivo.
npm start
```

Abra `http://localhost:3000`. Para apresentar sem instalar o compilador C ou o libpq, deixe PostgreSQL, Node.js, dependências e `.env` preparados antes; na apresentação, basta iniciar `npm start` e abrir o endereço local.

Não abra o HTML por `file://`: a API precisa estar em execução. Nunca coloque a senha em `visualizacao.html`, `app.js` ou em qualquer arquivo versionado.

Os testes da lógica de classificação podem ser executados com `npm test`.

### Regras de dados

- O programa trabalha com até 100 equipes e 1000 partidas por execução.
- Nomes de equipes podem conter espaços e devem ter até 100 caracteres.
- Os placares são armazenados em `Resultados.resultado_partida` (`DECIMAL(10,2)`).
- Cada partida precisa ter exatamente dois resultados para ser carregada no ranking.
- O código usa C99, libpq e os cabeçalhos locais em `persistencia/` e `modelos/`.

### Página web

O servidor Node entrega a interface em `http://localhost:3000` e atualiza o ranking e partidas a partir dos dados do PostgreSQL. A conexão é independente do programa C.
