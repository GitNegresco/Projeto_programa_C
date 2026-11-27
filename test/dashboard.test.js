const test = require('node:test');
const assert = require('node:assert/strict');
const { buildDashboard } = require('../dashboard');

const teams = [
  {
    id_equipe: 1,
    nome_equipe: 'Aurora',
    num_jogadores: 5,
    vitorias: 1,
    empates: 1,
    derrotas: 0,
    pontos: '4',
  },
  {
    id_equipe: 2,
    nome_equipe: 'Nébula',
    num_jogadores: 4,
    vitorias: 0,
    empates: 1,
    derrotas: 0,
    pontos: '1',
  },
  {
    id_equipe: 3,
    nome_equipe: 'Prisma',
    num_jogadores: 3,
    vitorias: 0,
    empates: 0,
    derrotas: 1,
    pontos: '0',
  },
];

test('builds standings and recent matches from paired result rows', () => {
  const rows = [
    {
      id_partida: 2,
      num_rodada: 2,
      id_resultado: 3,
      id_equipe: 1,
      placar: '1.50',
      kills: 12,
      mortes: 8,
      dano: '2500.00',
      cura_realizada: '300.00',
    },
    {
      id_partida: 2,
      num_rodada: 2,
      id_resultado: 4,
      id_equipe: 2,
      placar: '1.50',
      kills: 8,
      mortes: 12,
      dano: '1900.00',
      cura_realizada: '200.00',
    },
    {
      id_partida: 1,
      num_rodada: 1,
      id_resultado: 1,
      id_equipe: 1,
      placar: '2',
      kills: 15,
      mortes: 4,
      dano: '3000',
      cura_realizada: '100',
    },
    {
      id_partida: 1,
      num_rodada: 1,
      id_resultado: 2,
      id_equipe: 3,
      placar: '0',
      kills: 4,
      mortes: 15,
      dano: '1200',
      cura_realizada: '0',
    },
  ];

  const dashboard = buildDashboard(teams, rows);

  assert.deepEqual(dashboard.summary, {
    teams: 3,
    matches: 2,
    latestRound: 2,
  });
  assert.deepEqual(
    dashboard.ranking.map(({ name, points, wins, draws, losses }) => ({
      name, points, wins, draws, losses,
    })),
    [
      { name: 'Aurora', points: 4, wins: 1, draws: 1, losses: 0 },
      { name: 'Nébula', points: 1, wins: 0, draws: 1, losses: 0 },
      { name: 'Prisma', points: 0, wins: 0, draws: 0, losses: 1 },
    ],
  );
  assert.equal(dashboard.recentMatches[0].id, 2);
  assert.equal(dashboard.recentMatches[0].damageA, 2500);
});

test('rejects a match without exactly two distinct team results', () => {
  assert.throws(
    () => buildDashboard(teams, [{
      id_partida: 10,
      num_rodada: 5,
      id_resultado: 1,
      id_equipe: 1,
      placar: '1',
    }]),
    /duas equipes distintas/,
  );
});
