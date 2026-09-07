const BASE_URL = 'http://localhost:8000'

async function post(path, body) {
  const res = await fetch(`${BASE_URL}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  })
  if (!res.ok) throw new Error(`${path} failed: ${res.status}`)
  return res.json()
}

async function get(path) {
  const res = await fetch(`${BASE_URL}${path}`)
  if (!res.ok) throw new Error(`${path} failed: ${res.status}`)
  return res.json()
}

export const api = {
  reset: (config) => post('/api/reset', config),
  step: (steps) => post('/api/step', { steps }),
  state: () => get('/api/state'),
}
