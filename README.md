Projeto de Campeonato E-Sports

Integrantes:
- Christian Otto Ricarth (10253902)
- Felipe Negreiros Souza (10771620)
- João Manoel Pessoa Carneiro (10771808)
- Lucas Prado Fernandes Cardozo (10771939)

Descrição e principais funcionalidades:
Organizar e definir os jogos, equipes com seus jogadores, pontuações, partidas e chaves do campeonato. Vai poder visualizar as chaves, equipes e a estrutura do campeonato como um todo, cadastrar os jogadores nas equipes, definir qual jogo e qual partida será realizada e montar as chaves pra um campeonato.

Algoritmos implementados:
Linguagem C (projeto todo);
Structs e arrays(vetores) (para cadastro de jogadores, formação de partidas, definição de jogo, preenchimento de critérios);
Validações (para todos os dados inseridos nos campos corretos ou não);
Matriz ou estrutura parecida (claculo de pontuação do time talvez);
Busca bubble (para percorrer veoes que armazenem dados ou estatisticas do jogador);
Ir colocando aqui quando for fazendo o projeto...

Banco de dados e bilbioteca/driver escolhido:
Banco criado usando MySQL, e MYSQL Conncetor/C

Justificativa pra escolha desse banco ou biblioteca:
Informar quando for fazendo o projeto..

Estrutura de tabelas:
Entidades (Jogador, Equipe, Partida, Criterios_pontuacao, Jogo, Campenato);
jogador (id_jogador, nome completo, apelido, cpf, conta steam, telefone, funcao);
Equipe (id_equipe, nome, FK id_jogador, numero_jogadores, pontuacao);
Partida (FK id_equipes, id_partida, num_rodada);
Jogo (nome, desenvolvedora, tipo de jogo);
Criterios_pontuacao (vitoria, derrota, empate, num_kills, num_mortes, dano_cura_realizados);
Campeonato (nome, quant_partidas, quant_equipes, formato, FK id_partidas);

Instruções de Configuração, Compilação, e Execução:
Ir colocando aqui de acordo com o andamento do projeto...

Referências utilizadas na pesquisa:
Ir colocando aqui no decorrer do projeto.....
