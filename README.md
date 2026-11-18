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

Banco criado usando MySQL, e MYSQL Conncetor/C

Justificativa pra escolha desse banco ou biblioteca:

Informar quando for fazendo o projeto..

Estrutura de tabelas:

Entidades (Jogador, Equipe, Partida, Criterios_pontuacao, Jogo, Campenato);
jogador (CPF_id, nome completo, apelido, conta steam, telefone, funcao);
Equipe (id_equipe, nome, FK CPF_jogador, numero_jogadores, pontuacao);
Partida (FK id_equipes, id_partida, num_rodada, id_jogo);
Jogo (id_jogo, nome, desenvolvedora, tipo_jogo);
Criterios_pontuacao (vitoria, derrota, empate, num_kills, num_mortes, dano_cura_realizados);
Campeonato (nome, quant_partidas, quant_equipes, formato, FK id_partidas, FK id_equipes);

Instruções de Configuração, Compilação, e Execução:

Ir colocando aqui de acordo com o andamento do projeto...

Referências utilizadas na pesquisa:

Ir colocando aqui no decorrer do projeto.....
