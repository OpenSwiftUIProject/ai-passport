export const DEFAULT_SIMULATOR = 'https://openswiftuiproject.github.io/FoloToy-Passport-Simulator/';

export function simulatorURL(value, base) {
  const url = new URL(value, base);
  const loopback = ['127.0.0.1', 'localhost'].includes(url.hostname);
  if (!value.trim() || url.username || url.password || url.search || url.hash
      || (url.protocol !== 'https:' && !(url.protocol === 'http:' && loopback))) {
    throw new Error('Use an HTTPS Simulator URL or HTTP localhost, without credentials, query or fragment.');
  }
  if (!url.pathname.endsWith('/')) url.pathname += '/';
  return url;
}

export function simulatorTransport(target, pageURL) {
  if (target.origin === new URL(pageURL).origin) return 'discover';
  if (['127.0.0.1', 'localhost'].includes(target.hostname)) return 'http';
  return 'window';
}
