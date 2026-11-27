# Projeto de Campeonato E-Sports

## Integrantes

- Christian Otto Ricarth (10253902)
- Felipe Negreiros Souza (10771620)
- João Manoel Pessoa Carneiro (10771808)
- Lucas Prado Fernandes Cardozo (10771939)

## Descrição e principais funcionalidades

O projeto consiste no desenvolvimento de um sistema para organizar e gerenciar um campeonato de E-Sports no formato de pontos corridos.

O sistema permite organizar jogadores, equipes, partidas e pontuação do campeonato, além de possibilitar a consulta das informações e da classificação das equipes.

## Algoritmos implementados

- **Linguagem C:** utilizada no desenvolvimento do projeto.
- **Estruturas e vetores:** utilizados para armazenar informações de jogadores, equipes e partidas.
- **Validações de entrada:** funções para verificar os dados inseridos.
- **Busca linear:** utilizada para localizar jogadores pelo nickname e equipes pelo nome.
- **Busca binária:** prevista para localizar informações de forma otimizada.
- **Ordenação bolha:** prevista para organizar dados e auxiliar na apresentação das classificações.
- **Matrizes e estruturas:** utilizadas na organização das informações do campeonato.

## Banco de dados e biblioteca/driver escolhido

- Banco de dados: MySQL.
- Biblioteca/driver: MySQL Connector/C.
- Linguagem de integração: C.

## Justificativa para escolha desse banco ou biblioteca

O MySQL foi escolhido por ser um sistema de gerenciamento de banco de dados relacional, permitindo organizar e relacionar as informações de jogadores, equipes, partidas e campeonatos.

O MySQL Connector/C permite realizar a comunicação entre a aplicação desenvolvida em C e o banco de dados.

## Estrutura de tabelas

- **Jogador:** CPF ou identificador, nome completo, apelido, conta Steam, telefone e função.
- **Equipe:** identificador, nome, jogadores associados, quantidade de jogadores e pontuação.
- **Partida:** identificador, equipes participantes, número da rodada e jogo relacionado.
- **Jogo:** identificador, nome, desenvolvedora e tipo de jogo.
- **Critérios de pontuação:** vitórias, derrotas, empates, eliminações, mortes e dados de desempenho.
- **Campeonato:** nome, quantidade de partidas, quantidade de equipes, formato e partidas associadas.

## Instruções de configuração, compilação e execução

As instruções serão atualizadas de acordo com a estrutura final do projeto e a integração com o banco de dados MySQL.

## Testes

Serão realizados testes para verificar:

- Busca de jogadores existentes e inexistentes.
- Busca de equipes existentes e inexistentes.
- Validação de entradas numéricas.
- Validação de campos de texto vazios.
- Verificação de valores fora dos limites permitidos.

Os resultados serão registrados após a execução dos testes no programa integrado.

## Referências utilizadas na pesquisa

Próprio Curso de Algoritmos e Programação 2, Curso de Banco de dados, Documentação do PostgreSQL e ......
