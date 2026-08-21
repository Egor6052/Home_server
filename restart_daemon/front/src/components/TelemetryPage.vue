<template>
  <div class="telemetry-wrapper">
    <div class="telemetry-strip">
      <div class="status-dot" :class="freshnessClass" :title="freshnessLabel"></div>

      <div class="metric">
        <span class="metric-label">SRV</span>
        <span class="metric-value" :class="getTempClass(telemetry.serverTemperature)">
          {{ formatNumber(telemetry.serverTemperature) }}°C
        </span>
      </div>

      <span class="divider">|</span>

      <div class="metric">
        <span class="metric-label">EXT</span>
        <span class="metric-value" :class="getTempClass(telemetry.streetTemperature)">
          {{ formatNumber(telemetry.streetTemperature) }}°C
        </span>
      </div>

      <span class="divider">|</span>

      <div class="metric">
        <span class="metric-label">HUM</span>
        <span class="metric-value cyan">{{ formatNumber(telemetry.streetHumidity, 0) }}%</span>
      </div>

      <span class="divider">|</span>

      <div class="metric">
        <span class="metric-label">WDT</span>
        <span class="metric-value" :class="statusClass">{{ statusLabel }}</span>
        <span class="metric-sub">({{ telemetry.sttmRestartTimeoutSec ?? '--' }}s)</span>
      </div>

      <div class="strip-spacer"></div>

      <span class="last-update">{{ lastUpdateLabel }}</span>

      <button class="t-btn" :disabled="loading" @click="fetchTelemetry" title="Refresh now">
        {{ loading ? '...' : '⟳ Refresh' }}
      </button>
      <button class="t-btn danger" :disabled="restarting" @click="handleRestartSystem" title="Restart STM32 system">
        {{ restarting ? '...' : 'Restart' }}
      </button>
    </div>

    <p v-if="actionMessage" class="action-message" :class="{ 'is-error': actionIsError }">
      {{ actionMessage }}
    </p>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'

// const serverIP = window.location.hostname
// const baseUrl = computed(() => `http://${serverIP}`)

const telemetry = ref({
  serverTemperature: null,
  streetTemperature: null,
  streetHumidity: null,
  status: null,
  sttmRestartTimeoutSec: null,
  isFresh: false
})

const loading = ref(false)
const restarting = ref(false)
const actionMessage = ref('')
const actionIsError = ref(false)
const lastUpdate = ref(null)

let pollTimer = null

const fetchTelemetry = async () => {
  loading.value = true
  try {
    const res = await fetch('/api/telemetry')
    const data = await res.json()
    telemetry.value = data
    lastUpdate.value = new Date()
  } catch (e) {
    console.error('Failed to fetch telemetry', e)
  } finally {
    loading.value = false
  }
}

const handleRestartSystem = async () => {
  if (!confirm('Restart the STM32 controller now? The watchdog line will be held for a few seconds.')) {
    return
  }
  restarting.value = true
  actionMessage.value = ''
  try {
    const res = await fetch('/api/restart-system', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    })
    const data = await res.json()
    actionIsError.value = !data.success
    actionMessage.value = data.message || (data.success ? 'Command sent' : 'Failed to send command')
  } catch (e) {
    actionIsError.value = true
    actionMessage.value = 'Network error while sending restart command'
  } finally {
    restarting.value = false
  }
}

const formatNumber = (val, digits = 1) => (val !== null && val !== undefined ? Number(val).toFixed(digits) : '--')

const getTempClass = (t) => {
  if (t === null || t === undefined) return ''
  return t > 30 ? 'red' : t < 15 ? 'blue' : 'yellow'
}

const freshnessClass = computed(() => (telemetry.value.isFresh ? 'yellow' : 'red'))
const freshnessLabel = computed(() => (telemetry.value.isFresh ? 'LIVE' : 'STALE'))

const STATUS_LABELS = { 0: 'OK', 1: 'ERROR_V', 2: 'ERROR_T' }
const statusLabel = computed(() => STATUS_LABELS[telemetry.value.status] ?? `UNKNOWN (${telemetry.value.status})`)
const statusClass = computed(() => (telemetry.value.status === 0 ? 'yellow' : 'red'))

const lastUpdateLabel = computed(() => (lastUpdate.value ? lastUpdate.value.toLocaleTimeString('uk-UA') : '--:--'))

onMounted(() => {
  fetchTelemetry()
  pollTimer = setInterval(fetchTelemetry, 5000)
})

onUnmounted(() => {
  if (pollTimer) clearInterval(pollTimer)
})
</script>

<style scoped>
  @import "../assets/css/telemetry_page.css";
</style>