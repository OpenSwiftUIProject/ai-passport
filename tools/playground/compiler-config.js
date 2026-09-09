export const DEFAULT_COMPILER = 'http://127.0.0.1:4191/compile';

export function compilerURL(value, base) {
  const url = new URL(value, base);
  if (!value.trim() || url.username || url.password || url.search || url.hash) {
    throw new Error('Use a compiler URL without credentials, query parameters or fragments.');
  }
  const loopback = ['127.0.0.1', 'localhost'].includes(url.hostname);
  if (url.protocol !== 'https:' && !(url.protocol === 'http:' && loopback)) {
    throw new Error('Use HTTPS for a remote compiler, or HTTP localhost / 127.0.0.1.');
  }
  if (url.pathname === '/') url.pathname = '/compile';
  return url;
}

export function compilerFetchOptions(url) {
  return {
    credentials: 'omit', redirect: 'error', cache: 'no-store',
    // Supporting browsers can prompt for loopback access from an HTTPS page.
    ...(['127.0.0.1', 'localhost'].includes(url.hostname) ? { targetAddressSpace: 'loopback' } : {}),
  };
}
