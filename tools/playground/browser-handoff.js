// Protocol v1, shared with OpenSwiftUIProject/ai-passport/tools/playground.
// Keep the wire format compatible; each repository ships its own static copy.
const DATABASE = 'openswiftui-passport-handoff-v1';
const STORE = 'firmware';
export const HANDOFF_TTL = 10 * 60 * 1000;
const MAX_IMAGES = 3;
const MAX_BYTES = 8 * 1024 * 1024;
const ID = /^[a-f0-9]{48}$/;
const SHA = /^[a-f0-9]{64}$/;

export async function sha256(bytes) {
  return [...new Uint8Array(await crypto.subtle.digest('SHA-256', bytes))]
    .map(byte => byte.toString(16).padStart(2, '0')).join('');
}

export async function validateHandoff(record, id, destination, now = Date.now()) {
  if (!ID.test(id) || !record || record.id !== id || record.version !== 1) {
    throw new Error('Firmware link not found. Send it again from Playground in this browser.');
  }
  if (!Number.isSafeInteger(record.expiresAt) || record.expiresAt <= now
      || record.expiresAt > now + HANDOFF_TTL) {
    throw new Error('Firmware link expired. Send it again from Playground.');
  }
  if (record.destination !== destination || !SHA.test(record.sha256)
      || !SHA.test(record.sourceSha256) || !(record.bytes instanceof ArrayBuffer)
      || record.bytes.byteLength <= 0x10000 || record.bytes.byteLength > MAX_BYTES) {
    throw new Error('Invalid full firmware or Simulator destination.');
  }
  const image = new Uint8Array(record.bytes);
  if (image[0] !== 0xe9 || image[0x8000] !== 0xaa || image[0x8001] !== 0x50 || image[0x10000] !== 0xe9) {
    throw new Error('Invalid full firmware: expected bootloader, partition table and application.');
  }
  if (await sha256(record.bytes) !== record.sha256) throw new Error('Firmware checksum mismatch.');
  return record.bytes;
}

function database() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DATABASE, 1);
    request.onupgradeneeded = () => request.result.createObjectStore(STORE, { keyPath: 'id' });
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
    request.onblocked = () => reject(new Error('Close old Simulator tabs and retry.'));
  });
}

async function transaction(mode, action) {
  const db = await database();
  try {
    return await new Promise((resolve, reject) => {
      const tx = db.transaction(STORE, mode);
      let result;
      tx.oncomplete = () => resolve(result);
      tx.onerror = tx.onabort = () => reject(tx.error || new Error('Browser firmware storage failed.'));
      action(tx.objectStore(STORE), value => { result = value; });
    });
  } finally { db.close(); }
}

export async function stageFirmware(bytes, checksum, sourceSha256, destination) {
  const id = [...crypto.getRandomValues(new Uint8Array(24))]
    .map(byte => byte.toString(16).padStart(2, '0')).join('');
  const now = Date.now();
  const record = { version: 1, id, bytes, sha256: checksum, sourceSha256,
    destination, expiresAt: now + HANDOFF_TTL };
  await validateHandoff(record, id, destination, now);
  await transaction('readwrite', store => {
    const request = store.getAll();
    request.onsuccess = () => {
      const existing = request.result.sort((a, b) => b.expiresAt - a.expiresAt);
      existing.forEach((item, index) => {
        if (item.expiresAt <= now || index >= MAX_IMAGES - 1) store.delete(item.id);
      });
      store.put(record);
    };
  });
  return id;
}

export async function readFirmware(id, destination) {
  if (!ID.test(id)) throw new Error('Invalid Playground firmware link.');
  const record = await transaction('readonly', (store, result) => {
    const request = store.get(id);
    request.onsuccess = () => result(request.result);
  });
  return validateHandoff(record, id, destination);
}

export function removeFirmware(id) {
  return transaction('readwrite', store => { store.delete(id); });
}
