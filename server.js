const http = require('node:http');
const path = require('node:path');
const { readFile } = require('node:fs/promises');
require('dotenv').config();
const { Pool } = require('pg');
const { buildDashboard } = require('./dashboard');

const port = Number(process.env.PORT || 3000);
const hasDatabaseConfiguration = Boolean(
  process.env.DATABASE_URL || process.env.PGPASSWORD,
);
const databaseOptions = process.env.DATABASE_URL
  ? { connectionString: process.env.DATABASE_URL }
  : {
      host: process.env.PGHOST || 'localhost',
      port: Number(process.env.PGPORT || 5432),
      database: process.env.PGDATABASE || 'Campeonato_Esports',
      user: process.env.PGUSER,
      password: process.env.PGPASSWORD,
    };
const pool = hasDatabaseConfiguration
  ? new Pool({
      ...databaseOptions,
      max: 5,
      connectionTimeoutMillis: 5000,
    })
  : null;

const staticFiles = new Map([
  ['/', ['visualizacao.html', 'text/html; charset=utf-8']],
  ['/visualizacao.html', ['visualizacao.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
]);

function sendJson(response, status, value) {
  response.writeHead(status, {
    'Content-Type': 'application/json; charset=utf-8',
    'Cache-Control': 'no-store',
    'X-Content-Type-Options': 'nosniff',
  });
  response.end(JSON.stringify(value));
}

async function sendStatic(response, filename, contentType) {
  const contents = await readFile(path.join(__dirname, filename));
  response.writeHead(200, {
    'Content-Type': contentType,
    'Cache-Control': 'no-cache',
    'X-Content-Type-Options': 'nosniff',
  });
  response.end(contents);
}

async function loadDashboard() {
  const [teamResult, matchResult] = await Promise.all([
    pool.query(
      `SELECT e.id_equipe,
              COALESCE(e.nome_equipe, '') AS nome_equipe,
              (SELECT COUNT(*)::int
               FROM jogadores j
               WHERE j.fk_equipes_id_equipe = e.id_equipe) AS num_jogadores,
              COALESCE(p.vitorias, 0)::int AS vitorias,
              COALESCE(p.derrotas, 0)::int AS derrotas,
              COALESCE(p.empates, 0)::int AS empates,
              COALESCE(p.pontos, 0)::text AS pontos
       FROM equipes e
       LEFT JOIN (
         SELECT fk_equipes_id_equipe,
                SUM(COALESCE(n_vitoria, 0)) AS vitorias,
                SUM(COALESCE(n_derrota, 0)) AS derrotas,
                SUM(COALESCE(n_empate, 0)) AS empates,
                SUM(COALESCE(n_pontos, 0)) AS pontos
         FROM pontuacao
         GROUP BY fk_equipes_id_equipe
       ) p ON p.fk_equipes_id_equipe = e.id_equipe
       ORDER BY e.id_equipe`,
    ),
    pool.query(
      `SELECT p.id_partida,
              p.num_rodada,
              r.id_resultado,
              r.fk_equipes_id_equipe AS id_equipe,
              r.resultado_partida::text AS placar,
              r.kills,
              r.mortes,
              r.dano::text AS dano,
              r.cura_realizada::text AS cura_realizada
       FROM partidas p
       LEFT JOIN resultados r
         ON r.fk_partidas_id_partida = p.id_partida
       ORDER BY p.id_partida DESC, r.id_resultado`,
    ),
  ]);

  return buildDashboard(teamResult.rows, matchResult.rows);
}

const server = http.createServer(async (request, response) => {
  const url = new URL(request.url, 'http://localhost');

  if (request.method !== 'GET') {
    sendJson(response, 405, { error: 'Método não permitido.' });
    return;
  }

  if (url.pathname === '/api/health') {
    if (!pool) {
      sendJson(response, 503, { error: 'DATABASE_URL não configurada no servidor.' });
      return;
    }
    try {
      await pool.query('SELECT 1');
      sendJson(response, 200, { status: 'ok' });
    } catch (error) {
      console.error('Falha de conexão com o PostgreSQL:', error.message);
      sendJson(response, 503, { error: 'Não foi possível conectar ao PostgreSQL.' });
    }
    return;
  }

  if (url.pathname === '/api/dashboard') {
    if (!pool) {
      sendJson(response, 503, { error: 'DATABASE_URL não configurada no servidor.' });
      return;
    }
    try {
      sendJson(response, 200, await loadDashboard());
    } catch (error) {
      console.error('Falha ao carregar o painel:', error.message);
      sendJson(response, 500, { error: 'Falha ao carregar os dados do campeonato.' });
    }
    return;
  }

  const staticFile = staticFiles.get(url.pathname);
  if (staticFile) {
    try {
      await sendStatic(response, staticFile[0], staticFile[1]);
    } catch (error) {
      console.error('Falha ao ler arquivo estático:', error.message);
      sendJson(response, 500, { error: 'Falha ao abrir a página.' });
    }
    return;
  }

  sendJson(response, 404, { error: 'Rota não encontrada.' });
});

server.listen(port, '0.0.0.0', () => {
  console.log(`Aplicação disponível na porta ${port}.`);
  if (!pool) {
    console.warn('Configure DATABASE_URL para habilitar os dados do PostgreSQL.');
  }
});

function closeServer() {
  server.close(() => {
    if (pool) {
      pool.end().finally(() => process.exit(0));
    } else {
      process.exit(0);
    }
  });
}

process.on('SIGINT', closeServer);
process.on('SIGTERM', closeServer);
