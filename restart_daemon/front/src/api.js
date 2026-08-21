// Тонкий шар над fetch. Всі шляхи відносні ('/api/...'), тому що фронт
// віддається тим самим httplib-сервером, який обробляє ці ж запити -
// той самий origin, окремий CORS-конфіг не потрібен.
async function getJson(url, options) {
  const res = await fetch(url, options);
  let body = null;
  try {
    body = await res.json();
  } catch (e) {
    body = null;
  }
  return { ok: res.ok, status: res.status, body };
}

async function postJson(url, payload) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload)
  });
  let body = null;
  try {
    body = await res.json();
  } catch (e) {
    body = null;
  }
  return { ok: res.ok, status: res.status, body };
}

// ---------- BlackBox ----------
export async function getBlackBoxSettings() {
  const { body } = await getJson('/api/blackbox-settings');
  return body || {};
}

export async function setBlackBoxSettings(path) {
  const { body } = await postJson('/api/blackbox-settings', { path });
  return body || { status: 'error', message: 'Порожня відповідь сервера' };
}

export async function listBlackBoxFiles() {
  const { body } = await getJson('/api/blackbox-all');
  return (body && body.files) || [];
}

export async function getBlackBoxFileContent(name) {
  const { ok, body } = await getJson(`/api/blackbox-file?name=${encodeURIComponent(name)}`);
  if (!ok) {
    throw new Error((body && body.message) || 'Не вдалося завантажити вміст файлу');
  }
  return (body && body.content) || '';
}

export async function removeBlackBoxFile(name) {
  const { body } = await postJson('/api/blackbox-remove-file', { name });
  return body || { status: 'error', message: 'Порожня відповідь сервера' };
}

export function blackBoxDownloadUrl(name) {
  return `/api/blackbox-download?name=${encodeURIComponent(name)}`;
}