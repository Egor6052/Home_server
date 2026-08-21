<template>
  <div class="settings-modal-overlay" @click.self="$emit('close')">
    <div class="settings-modal">
      <div class="modal-header">
        <h3>System Settings</h3>
        <button class="close-btn" @click="$emit('close')">✕</button>
      </div>

      <div class="modal-body">
        <section class="settings-section">
          <div class="section-label">STM32 Watchdog</div>

          <div class="field-row">
            <label>Restart timeout (sec)</label>
            <input type="number" v-model.number="newTimeSecRestart" min="5" max="254" />
          </div>

          <div class="section-footer">
            <button class="save-btn" :disabled="stmStatus === 'saving'" @click="saveStmSettings">
              {{ stmStatus === 'saving' ? 'Saving...' : 'Save' }}
            </button>
            <span v-if="stmStatus === 'success'" class="status-ok">Saved</span>
            <span v-if="stmStatus === 'error'" class="status-error">Failed to save</span>
          </div>
        </section>

        <section class="settings-section">
          <div class="section-label">Backend Connection</div>

          <div class="field-row">
            <label>IP</label>
            <input type="text" v-model="configIp" />
          </div>
          <div class="field-row">
            <label>Port</label>
            <input type="number" v-model.number="configPort" min="1" max="65535" />
          </div>

          <div class="section-footer">
            <button class="save-btn" :disabled="rsStatus === 'saving'" @click="saveRsConfig">
              {{ rsStatus === 'saving' ? 'Saving...' : 'Save' }}
            </button>
            <span v-if="rsStatus === 'success'" class="status-ok">Saved</span>
            <span v-if="rsStatus === 'error'" class="status-error">Failed to save</span>
          </div>
        </section>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'

const newTimeSecRestart = ref(60)
const configIp = ref('127.0.0.1')
const configPort = ref(6060)

const stmStatus = ref('idle') // idle | saving | success | error
const rsStatus = ref('idle')

async function fetchStmSettings() {
  try {
    const res = await fetch('/api/get-stm-settings')
    const data = await res.json()
    if (data.new_time_sec_restart !== undefined) {
      newTimeSecRestart.value = Number(data.new_time_sec_restart)
    }
  } catch (e) {
    console.error('Failed to load STM settings:', e)
  }
}

async function fetchRsConfig() {
  try {
    const res = await fetch('/api/rs-config')
    const data = await res.json()
    if (data.config_ip !== undefined) configIp.value = data.config_ip
    if (data.config_port !== undefined) configPort.value = Number(data.config_port)
  } catch (e) {
    console.error('Failed to load backend config:', e)
  }
}

async function saveStmSettings() {
  stmStatus.value = 'saving'
  try {
    const res = await fetch('/api/change-stm-settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ new_time_sec_restart: Math.round(newTimeSecRestart.value) })
    })
    const data = await res.json()
    stmStatus.value = res.ok && data.success ? 'success' : 'error'
  } catch (e) {
    console.error('Failed to save STM settings:', e)
    stmStatus.value = 'error'
  }
  setTimeout(() => { stmStatus.value = 'idle' }, 2000)
}

async function saveRsConfig() {
  rsStatus.value = 'saving'
  try {
    const res = await fetch('/api/rs-config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ config_ip: configIp.value, config_port: Math.round(configPort.value) })
    })
    const data = await res.json()
    rsStatus.value = res.ok && data.status === 'success' ? 'success' : 'error'
  } catch (e) {
    console.error('Failed to save backend config:', e)
    rsStatus.value = 'error'
  }
  setTimeout(() => { rsStatus.value = 'idle' }, 2000)
}

onMounted(() => {
  fetchStmSettings()
  fetchRsConfig()
})
</script>

<style scoped>
.settings-modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(2px);
  z-index: 2200;
  display: flex;
  align-items: center;
  justify-content: center;
}

.settings-modal {
  width: 420px;
  max-width: 90vw;
  background: #0d0d0d;
  border: 1px solid rgba(255, 234, 0, 0.2);
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.8);
  font-family: 'JetBrains Mono', monospace;
  color: #fff;
}

.modal-header {
  padding: 16px 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 1px solid #222;
}

.modal-header h3 {
  margin: 0;
  color: #ffea00;
  font-size: 14px;
  letter-spacing: 1px;
  text-transform: uppercase;
}

.modal-header .close-btn {
  background: none;
  border: none;
  color: #888;
  font-size: 18px;
  cursor: pointer;
}

.modal-header .close-btn:hover {
  color: #ffea00;
}

.modal-body {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.settings-section .section-label {
  font-size: 10px;
  color: #555;
  text-transform: uppercase;
  margin-bottom: 10px;
  letter-spacing: 1px;
}

.settings-section .field-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 10px;
}

.settings-section .field-row label {
  font-size: 12px;
  color: #ccc;
}

.settings-section .field-row input {
  width: 140px;
  background: #000;
  border: 1px solid #333;
  color: #fff;
  padding: 6px 8px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 12px;
}

.settings-section .field-row input:focus {
  outline: none;
  border-color: #ffea00;
}

.settings-section .section-footer {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-top: 6px;
}

.settings-section .section-footer .save-btn {
  background: transparent;
  border: 1px solid #ffea00;
  color: #ffea00;
  padding: 6px 16px;
  font-family: 'JetBrains Mono', monospace;
  font-size: 11px;
  text-transform: uppercase;
  cursor: pointer;
  transition: all 0.2s ease;
}

.settings-section .section-footer .save-btn:hover:not(:disabled) {
  background: #ffea00;
  color: #000;
}

.settings-section .section-footer .save-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.settings-section .section-footer .status-ok {
  color: #6fcf97;
  font-size: 11px;
}

.settings-section .section-footer .status-error {
  color: #e74c3c;
  font-size: 11px;
}
</style>