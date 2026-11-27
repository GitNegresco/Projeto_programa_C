const rankingBody = document.querySelector('#ranking-body');
const recentMatches = document.querySelector('#recent-matches');
const teamsCount = document.querySelector('#teams-count');
const matchesCount = document.querySelector('#matches-count');
const latestRound = document.querySelector('#latest-round');

function formatNumber(value) {
  if (value === null || value === undefined) {
    return '—';
  }
  return new Intl.NumberFormat('pt-BR', {
    maximumFractionDigits: 2,
  }).format(value);
}

function setTableMessage(message) {
  rankingBody.replaceChildren();
  const row = document.createElement('tr');
  const cell = document.createElement('td');
  cell.colSpan = 7;
  cell.className = 'px-5 py-4 text-slate-500';
  cell.textContent = message;
  row.append(cell);
  rankingBody.append(row);
}

function renderRanking(ranking) {
  rankingBody.replaceChildren();
  if (ranking.length === 0) {
    setTableMessage('Nenhuma equipe cadastrada.');
    return;
  }

  ranking.forEach((team, index) => {
    const row = document.createElement('tr');
    row.className = index === 0 ? 'bg-blue-50/70' : 'hover:bg-slate-50';
    const values = [
      `${index + 1}º`,
      team.name,
      team.games,
      team.wins,
      team.draws,
      team.losses,
      team.points,
    ];

    values.forEach((value, cellIndex) => {
      const cell = document.createElement('td');
      cell.textContent = String(value);
      if (cellIndex === 0) {
        cell.className = index === 0
          ? 'px-5 py-4 font-semibold text-blue-700'
          : 'px-5 py-4 text-slate-500';
      } else if (cellIndex === 1) {
        cell.className = 'px-3 py-4 font-medium text-slate-900';
      } else if (cellIndex === 6) {
        cell.className = 'px-5 py-4 text-right font-semibold text-slate-900';
      } else {
        cell.className = 'px-3 py-4 text-center text-slate-600';
      }
      row.append(cell);
    });
    rankingBody.append(row);
  });
}

function createMatchArticle(match) {
  const article = document.createElement('article');
  article.className = 'px-5 py-4';

  const scoreLine = document.createElement('div');
  scoreLine.className = 'flex items-center justify-between gap-3';
  const names = document.createElement('p');
  names.className = 'font-medium text-slate-900';
  names.textContent = `${match.teamA} x ${match.teamB}`;
  const score = document.createElement('p');
  score.className = 'whitespace-nowrap rounded-md bg-slate-100 px-3 py-1.5 font-semibold text-slate-800';
  score.textContent = `${formatNumber(match.scoreA)} x ${formatNumber(match.scoreB)}`;
  scoreLine.append(names, score);

  const combatStats = document.createElement('p');
  combatStats.className = 'mt-2 text-xs text-slate-500';
  combatStats.textContent =
    `Rodada ${match.round} · Kills: ${formatNumber(match.killsA)} x ${formatNumber(match.killsB)}` +
    ` · Mortes: ${formatNumber(match.deathsA)} x ${formatNumber(match.deathsB)}`;

  const supportStats = document.createElement('p');
  supportStats.className = 'mt-1 text-xs text-slate-500';
  supportStats.textContent =
    `Dano: ${formatNumber(match.damageA)} x ${formatNumber(match.damageB)}` +
    ` · Cura: ${formatNumber(match.healingA)} x ${formatNumber(match.healingB)}`;

  article.append(scoreLine, combatStats, supportStats);
  return article;
}

function renderRecentMatches(matches) {
  recentMatches.replaceChildren();
  if (matches.length === 0) {
    const message = document.createElement('p');
    message.className = 'px-5 py-4 text-sm text-slate-500';
    message.textContent = 'Nenhuma partida registrada.';
    recentMatches.append(message);
    return;
  }
  matches.forEach((match) => recentMatches.append(createMatchArticle(match)));
}

async function loadDashboard() {
  setTableMessage('Carregando ranking...');
  try {
    const response = await fetch('/api/dashboard', {
      headers: { Accept: 'application/json' },
    });
    const dashboard = await response.json();
    if (!response.ok) {
      throw new Error(dashboard.error || 'Não foi possível carregar o painel.');
    }

    teamsCount.textContent = String(dashboard.summary.teams);
    matchesCount.textContent = String(dashboard.summary.matches);
    latestRound.textContent = String(dashboard.summary.latestRound);
    renderRanking(dashboard.ranking);
    renderRecentMatches(dashboard.recentMatches);
  } catch (error) {
    setTableMessage(error.message);
    recentMatches.replaceChildren();
    const message = document.createElement('p');
    message.className = 'px-5 py-4 text-sm text-rose-700';
    message.textContent = error.message;
    recentMatches.append(message);
  }
}

loadDashboard();
