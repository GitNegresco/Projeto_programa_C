# Projeto de Campeonato E-Sports

## Integrantes

- Christian Otto Ricarth (10253902)
- Felipe Negreiros Souza (10771620)
- João Manoel Pessoa Carneiro (10771808)
- Lucas Prado Fernandes Cardozo (10771939)

## Descrição e principais funcionalidades

O projeto consiste no desenvolvimento de um sistema para organizar e gerenciar um campeonato de E-Sports no formato de pontos corridos, podendo ser escolhido o jogo e cadastrados e montadas as equipes no próprio sistema.

O sistema permite organizar jogadores, equipes, partidas e pontuação do campeonato, além de possibilitar a consulta das pontuações e da classificação das equipes, além é claro de ter esses campos com validação.

Condicionais definidas pro sistema:
Um jogador pertence a no máximo uma equipe. Remover a equipe deixa o jogador sem equipe.
Uma equipe que já jogou partidas não pode ser removida.
Vitória vale 3 pontos, empate 1 e derrota 0.
Kills, mortes, dano e cura são totais da equipe em cada partida.
A pontuacao é dado derivado de resultados e não deve ser alterada à mão.
Os identificadores (id) são gerados pelo banco.
Partidas sem campeonato ou jogo informados usam o campeonato e o jogo padrão criados pelo script.
A ordenação do ranking é feita no programa em C, e não no banco.

## Algoritmos implementados

- **Linguagem C:** utilizada no desenvolvimento do projeto.
- **Estruturas e vetores:** utilizados para armazenar informações de jogadores, equipes e partidas.
- **Validações de entrada:** funções para verificar os dados inseridos.
- **Busca linear:** utilizada para localizar jogadores pelo nickname e equipes pelo nome.
- **Busca binária:** prevista para localizar informações de forma otimizada.
- **Ordenação bolha:** prevista para organizar dados e auxiliar na apresentação das classificações.
- **Matrizes e estruturas:** utilizadas na organização das informações do campeonato.

## Banco de dados e biblioteca/driver escolhido

- Banco de dados: PostgreSQL (schema testado no PostgreSQL 16 e executado no 19; funciona a partir da versão 14).
- Biblioteca/driver: libpq, a biblioteca oficial em C do PostgreSQL (libpq-fe.h).
- Linguagem de integração: C.

## Justificativa para escolha desse banco ou biblioteca

O PostgreSQL garante a integridade dos dados no próprio banco: chaves estrangeiras, UNIQUE e CHECK impedem, por exemplo, a mesma equipe duas vezes na mesma partida ou um placar negativo.
A tabela pontuacao é mantida automaticamente por trigger, então o total de cada equipe se corrige sozinho quando um placar é alterado.
A libpq é o driver oficial para C, instalado junto com o PostgreSQL, sem dependências extras.
O banco é criado a partir de um único script (persistencia/schema.sql), o que torna a instalação reproduzível em qualquer computador.

## Estrutura de tabelas

- **Jogador:** CPF, identificador, nome completo, apelido, conta Steam, telefone e função.
- **Equipe:** identificador, nome, jogadores associados, quantidade de jogadores e pontuação.
- **Partida:** identificador, equipes participantes, número da rodada e jogo relacionado.
- **Jogo:** identificador, nome, desenvolvedora e tipo de jogo.
- **Critérios de pontuação:** vitórias, derrotas, empates, eliminações, mortes e dados de desempenho.
- **Campeonato:** nome, quantidade de partidas, quantidade de equipes, formato e partidas associadas.

## Instruções de configuração, compilação e execução


Utilização do banco de dados Postgre:
1. Instalar o PostgreSQL
Instale o PostgreSQL (versão 14 ou superior) com o instalador oficial e anote a senha do usuário postgres. Mantenha a porta padrão (5432). O instalador já inclui o pgAdmin e a libpq. No caso do Linux, instale separadamente o pacote libpq-dev.

2. Criar o banco Campeonato_Esports
O nome precisa ser escrito exatamente assim, com maiúsculas.
Pelo pgAdmin: botão direito em Databases → Create → Database... → nome Campeonato_Esports → Save. (modo recomendado e feito assim pela equipe)
Se for pelo terminal use o comando:
createdb -U postgres Campeonato_Esports

3. Carregar o script
Pelo pgAdmin: selecione o banco Campeonato_Esports, abra Tools → Query Tool, confirme que o título da aba mostra Campeonato_Esports, clique em Open File, escolha persistencia/schema.sql e execute (F5).
Se for pelo terminal use o comando:
psql -U postgres -d Campeonato_Esports -f persistencia/schema.sql

Ao terminar, devem existir 8 tabelas e a view classificacao.
Atenção: rodar o script de novo recria as tabelas e apaga todos os dados. (verificar como resgatar esses dados depois)

4. Definir as variáveis de ambiente
O programa lê o acesso ao banco das variáveis de ambiente. A senha nunca fica no código! (Execute um comando por vez)
Windows (PowerShell):
$env:PGUSER="postgres"
$env:PGPASSWORD="sua_senha"
$env:PGDATABASE="Campeonato_Esports"

Linux:
export PGUSER=postgres
export PGPASSWORD=sua_senha
export PGDATABASE=Campeonato_Esports

Se o PostgreSQL usar outra porta, defina também PGPORT. (no caso usamos a 5432 msm)

5. Compilar
Substitua <arquivos .c do projeto> pelos arquivos-fonte do projeto, incluindo persistencia/banco.c, e ajuste o número da versão do PostgreSQL se necessário. (obs o MinGW-w64 foi instalado a parte, caso não tenha instale-o também)
Windows (MinGW-w64 / MSYS2 UCRT64):
gcc <arquivos .c do projeto> -o campeonato.exe -I "C:/Program Files/PostgreSQL/19/include" "C:/Program Files/PostgreSQL/19/bin/libpq.dll"

Linux:
gcc <arquivos .c do projeto> -o campeonato -I$(pg_config --includedir) -lpq

6. Executar
Windows (PowerShell): o programa precisa encontrar a libpq.dll.
$env:Path += ";C:\Program Files\PostgreSQL\19\bin"
.\campeonato.exe

Linux:
./campeonato

## Testes

Serão realizados testes para verificar:

- Busca de jogadores existentes e inexistentes.
- Busca de equipes existentes e inexistentes.
- Validação de entradas numéricas.
- Validação de campos de texto vazios.
- Verificação de valores fora dos limites permitidos.

Os resultados serão registrados após a execução dos testes no programa integrado.

## Referências utilizadas na pesquisa

- Próprio Curso de Algoritmos e Programação 2.
- Curso de Banco de dados.
- Documentação oficial do PostgreSQL.
- Documentação oficial do libpq.
