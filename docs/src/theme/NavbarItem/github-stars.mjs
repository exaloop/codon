const CACHE_KEY = 'codon.github-stars';
const CACHE_TTL_MS = 60 * 60 * 1000;
const REQUEST_TIMEOUT_MS = 5000;

function isCount(value) {
  return Number.isSafeInteger(value) && value >= 0;
}

export function formatStarCount(count) {
  return new Intl.NumberFormat('en-US', {notation: 'compact', maximumFractionDigits: 1})
    .format(count).toLowerCase();
}

export async function loadStarCount({fetch = globalThis.fetch, storage, now = Date.now()} = {}) {
  let cached;
  try {
    const record = JSON.parse(storage?.getItem(CACHE_KEY) || 'null');
    if (record && isCount(record.count) && Number.isFinite(record.at)) cached = record;
  } catch {}
  if (cached && now >= cached.at && now - cached.at < CACHE_TTL_MS) return cached.count;

  try {
    const response = await fetch('https://api.github.com/repos/exaloop/codon', {
      headers: {Accept: 'application/vnd.github+json'},
      signal: AbortSignal.timeout(REQUEST_TIMEOUT_MS),
    });
    if (!response.ok) throw new Error('GitHub star count unavailable');
    const count = (await response.json()).stargazers_count;
    if (!isCount(count)) throw new Error('Invalid GitHub star count');
    try {
      storage?.setItem(CACHE_KEY, JSON.stringify({count, at: now}));
    } catch {}
    return count;
  } catch {
    return cached?.count ?? null;
  }
}
