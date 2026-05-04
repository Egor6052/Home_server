<template>
  <div class="config-wrapper">
    <div class="header-section">
      <!-- <div class="status-dot" :class="{ 'pulse-yellow': !loading, 'pulse-red': loading }"></div>
      <h1>CLIMATE_CONTROL_UNIT</h1>
      <div class="header-spacer"></div> -->
    </div>

    <div class="main-layout">
      <div class="data-column">
        <div class="tactical-grid">
          
          <!-- TEMPERATURE CARD -->
          <section class="config-card">
            <h2 class="section-title">TEMPERATURE</h2>
            <div class="card-content">
              <div class="hud-value" :class="getTempClass(latestMeasurement?.temp)">
                {{ formatNumber(latestMeasurement?.temp) }}°C
              </div>
              
              <!-- Графік-іскорка (Sparkline) -->
              <div class="sparkline-container">
                <svg viewBox="0 0 100 30" preserveAspectRatio="none" class="sparkline">
                  <path :d="tempPath" fill="none" stroke="currentColor" stroke-width="1.5" :class="getTempClass(latestMeasurement?.temp)" />
                </svg>
              </div>

              <!-- Шкала (Linear Gauge) -->
              <div class="hud-gauge">
                <div class="gauge-bar" :style="{ width: tempPercent + '%' }" :class="getTempClass(latestMeasurement?.temp)"></div>
                <div class="gauge-labels">
                  <span>0°C</span><span>50°C</span>
                </div>
              </div>
            </div>
          </section>

          <!-- HUMIDITY CARD -->
          <section class="config-card">
            <h2 class="section-title">HUMIDITY</h2>
            <div class="card-content">
              <div class="hud-value cyan">
                {{ formatNumber(latestMeasurement?.hum) }}%
              </div>

              <div class="sparkline-container">
                <svg viewBox="0 0 100 30" preserveAspectRatio="none" class="sparkline">
                  <path :d="humPath" fill="none" stroke="#00f2ff" stroke-width="1.5" />
                </svg>
              </div>

              <div class="hud-gauge">
                <div class="gauge-bar cyan-bg" :style="{ width: (latestMeasurement?.hum || 0) + '%' }"></div>
                <div class="gauge-labels">
                  <span>0%</span><span>100%</span>
                </div>
              </div>
            </div>
          </section>

          <!-- HISTORY TABLE -->
          <section class="config-card full-width">
            <h2 class="section-title">HISTORY_BUFFER_24H</h2>
            <div class="table-container">
              <table class="t-table">
                <thead>
                  <tr>
                    <th>TIMESTAMP</th>
                    <th>TEMP</th>
                    <th>HUMIDITY</th>
                    <th>TREND</th>
                  </tr>
                </thead>
                <tbody>
                  <tr v-for="item in limitedHistory" :key="item.time">
                    <td>{{ formatUnixDate(item.time) }}</td>
                    <td :class="getTempClass(item.temp)">{{ formatNumber(item.temp) }}°C</td>
                    <td class="cyan">{{ formatNumber(item.hum) }}%</td>
                    <td>
                      <span class="trend-line" :style="{ width: (item.temp * 2) + 'px' }" :class="getTempClass(item.temp)"></span>
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          </section>
        </div>
      </div>

      <div class="camera-column">
        <CameraModule 
          :baseUrl="baseUrl" 
          :serverIP="serverIP" 
          camPort="8080" 
          @syncData="fetchData" 
        />
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import CameraModule from '../components/CameraModule.vue'

const measurements = ref([])
const loading = ref(false)
const serverIP = window.location.hostname
const apiPort = "1616"

const baseUrl = computed(() => `http://${serverIP}:${apiPort}`)

const fetchData = async () => {
  if (loading.value) return
  loading.value = true
  try {
    const res = await fetch(`${baseUrl.value}/api/24data`)
    const response = await res.json()
    if (response?.status === "success") {
      measurements.value = response.data
    }
  } catch (e) {
    console.error('API failure')
  } finally {
    loading.value = false
  }
}

const sortedMeasurements = computed(() => [...measurements.value].sort((a, b) => b.time - a.time))
const limitedHistory = computed(() => sortedMeasurements.value.slice(0, 20))
const latestMeasurement = computed(() => sortedMeasurements.value[0] || null)

// Логіка побудови графіків (SVG Path)
const generatePath = (data, key, minVal, maxVal) => {
  if (data.length < 2) return ""
  const points = data.slice(0, 20).reverse() // беремо останні 20
  const width = 100
  const height = 30
  
  return points.map((p, i) => {
    const x = (i / (points.length - 1)) * width
    // Масштабуємо значення відносно мін/макс
    const y = height - ((p[key] - minVal) / (maxVal - minVal)) * height
    return `${i === 0 ? 'M' : 'L'} ${x} ${y}`
  }).join(' ')
}

const tempPath = computed(() => generatePath(measurements.value, 'temp', 0, 45))
const humPath = computed(() => generatePath(measurements.value, 'hum', 0, 100))

// Відсоток для шкали (0-50 градусів)
const tempPercent = computed(() => {
  const t = latestMeasurement.value?.temp || 0
  return Math.min(Math.max((t / 50) * 100, 0), 100)
})

const formatNumber = (val) => (val !== undefined ? parseFloat(val).toFixed(1) : '--')
const formatUnixDate = (u) => u ? new Date(u * 1000).toLocaleTimeString('uk-UA') : '--:--'
const getTempClass = (t) => (t > 30 ? 'red' : t < 15 ? 'blue' : 'yellow')

onMounted(() => {
  fetchData()
  setInterval(fetchData, 15000)
})
</script>

<style scoped>
  @import "../assets/css/data_page.css";
</style>