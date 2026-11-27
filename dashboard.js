function requiredNumber(value, field) {
  if (value === null || value === undefined || value === '') {
    throw new Error(`Campo obrigatório ausente: ${field}`);
  }

  const number = Number(value);
  if (!Number.isFinite(number)) {
    throw new Error(`Valor inválido no campo: ${field}`);
  }
  return number;
}

function optionalNumber(value) {
  if (value === null || value === undefined || value === '') {
    return null;
  }

  const number = Number(value);
  return Number.isFinite(number) ? number : null;
}

function buildDashboard(teamRows, matchRows) {
  const teams = teamRows.map((row) => ({
    id: requiredNumber(row.id_equipe, 'id_equipe'),
    name: row.nome_equipe || `Equipe ${row.id_equipe}`,
    players: requiredNumber(row.num_jogadores, 'num_jogadores'),
    wins: requiredNumber(row.vitorias, 'n_vitoria'),
    draws: requiredNumber(row.empates, 'n_empate'),
    losses: requiredNumber(row.derrotas, 'n_derrota'),
    points: requiredNumber(row.pontos, 'n_pontos'),
  }));
  const teamById = new Map(teams.map((team) => [team.id, team]));
  const matchesById = new Map();

  for (const row of matchRows) {
    const matchId = requiredNumber(row.id_partida, 'id_partida');
    let match = matchesById.get(matchId);
    if (!match) {
      match = {
        id: matchId,
        round: optionalNumber(row.num_rodada) || 0,
        results: [],
      };
      matchesById.set(matchId, match);
    }

    if (row.id_resultado === null || row.id_resultado === undefined) {
      continue;
    }

    const teamId = requiredNumber(row.id_equipe, 'id_equipe');
    const team = teamById.get(teamId);
    if (!team) {
      throw new Error(`Resultado referencia equipe inexistente: ${teamId}`);
    }
    match.results.push({
      team,
      score: requiredNumber(row.placar, 'resultado_partida'),
      kills: optionalNumber(row.kills),
      deaths: optionalNumber(row.mortes),
      damage: optionalNumber(row.dano),
      healing: optionalNumber(row.cura_realizada),
    });
  }

  const statsById = new Map(teams.map((team) => [team.id, {
    ...team,
    games: team.wins + team.draws + team.losses,
    pointsFor: 0,
    pointsAgainst: 0,
  }]));
  const matches = [];

  for (const match of matchesById.values()) {
    if (match.results.length !== 2 ||
        match.results[0].team.id === match.results[1].team.id) {
      throw new Error(
        `A partida ${match.id} precisa ter resultados de duas equipes distintas.`,
      );
    }

    const [resultA, resultB] = match.results;
    const statsA = statsById.get(resultA.team.id);
    const statsB = statsById.get(resultB.team.id);
    statsA.pointsFor += resultA.score;
    statsA.pointsAgainst += resultB.score;
    statsB.pointsFor += resultB.score;
    statsB.pointsAgainst += resultA.score;

    matches.push({
      id: match.id,
      round: match.round,
      teamA: resultA.team.name,
      teamB: resultB.team.name,
      scoreA: resultA.score,
      scoreB: resultB.score,
      killsA: resultA.kills,
      killsB: resultB.kills,
      deathsA: resultA.deaths,
      deathsB: resultB.deaths,
      damageA: resultA.damage,
      damageB: resultB.damage,
      healingA: resultA.healing,
      healingB: resultB.healing,
    });
  }

  const ranking = Array.from(statsById.values())
    .map((team) => ({
      ...team,
      balance: team.pointsFor - team.pointsAgainst,
    }))
    .sort((a, b) =>
      b.points - a.points ||
      b.wins - a.wins ||
      b.balance - a.balance ||
      b.pointsFor - a.pointsFor ||
      a.id - b.id);

  const recentMatches = matches.slice(0, 5);
  return {
    summary: {
      teams: teams.length,
      matches: matches.length,
      latestRound: matches.reduce(
        (latest, match) => Math.max(latest, match.round),
        0,
      ),
    },
    ranking,
    recentMatches,
  };
}

module.exports = { buildDashboard };
