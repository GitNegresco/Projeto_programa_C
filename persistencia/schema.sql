CREATE TABLE Equipes (
    id_equipe INT NOT NULL,
    Nome_equipe VARCHAR(100),
    Num_jogadores INT,
    PRIMARY KEY (id_equipe)
);

CREATE TABLE Jogos (
    id_jogo INT NOT NULL,
    Nome VARCHAR(100),
    Desenvolvedora VARCHAR(100),
    Tipo_jogo VARCHAR(500),
    PRIMARY KEY (id_jogo)
);

CREATE TABLE Campeonato (
    id_campeonato INT NOT NULL,
    Nome_campeonato VARCHAR(100),
    Formato VARCHAR(50),
    fk_Equipes_id_equipe INT,
    PRIMARY KEY (id_campeonato),
    FOREIGN KEY (fk_Equipes_id_equipe) REFERENCES Equipes(id_equipe)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE Jogadores (
    CPF INT NOT NULL,
    Nome_completo VARCHAR(100),
    Apelido VARCHAR(50),
    Conta_steam VARCHAR(100),
    Telefone NUMERIC,
    fk_Equipes_id_equipe INT,
    PRIMARY KEY (CPF),
    FOREIGN KEY (fk_Equipes_id_equipe) REFERENCES Equipes(id_equipe)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE Pontuacao (
    id_pontuacao INT NOT NULL,
    N_vitoria INT,
    N_derrota INT,
    N_empate INT,
    N_kills INT,
    N_mortes INT,
    Dado_total DECIMAL(10, 2),
    Cura_total DECIMAL(10, 2),
    N_pontos DECIMAL(10, 2),
    fk_Equipes_id_equipe INT NOT NULL,
    PRIMARY KEY (id_pontuacao),
    FOREIGN KEY (fk_Equipes_id_equipe) REFERENCES Equipes(id_equipe)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE Partidas (
    id_partida INT NOT NULL,
    Num_rodada INT,
    fk_Campeonato_id_campeonato INT NOT NULL,
    fk_Jogos_id_jogo INT,
    PRIMARY KEY (id_partida),
    FOREIGN KEY (fk_Campeonato_id_campeonato) REFERENCES Campeonato(id_campeonato)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT,
    FOREIGN KEY (fk_Jogos_id_jogo) REFERENCES Jogos(id_jogo)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);

CREATE TABLE Resultados (
    id_resultado INT NOT NULL,
    resultado_partida DECIMAL(10, 2),
    kills INT,
    mortes INT,
    dano DECIMAL(10, 2),
    cura_realizada DECIMAL(10, 2),
    fk_Equipes_id_equipe INT,
    fk_Partidas_id_partida INT NOT NULL,
    PRIMARY KEY (id_resultado),
    FOREIGN KEY (fk_Equipes_id_equipe) REFERENCES Equipes(id_equipe)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT,
    FOREIGN KEY (fk_Partidas_id_partida) REFERENCES Partidas(id_partida)
        ON DELETE RESTRICT
        ON UPDATE RESTRICT
);
