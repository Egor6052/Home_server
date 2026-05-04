<template>
    <section class="config-card camera-card">
    <!-- HEADER КЕРУВАННЯ -->
    <div class="camera-header">
        <div class="title-group">
            <h2 class="section-title">OPTICAL_FEED_01</h2>
            <div class="status-indicator" :class="{ 'live': isStreaming }">
                <div class="dot"></div>
                <span>{{ isStreaming ? 'LIVE_STREAM' : 'STANDBY' }}</span>
            </div>
        </div>
        <div class="actions">
            <button @click="searchCameras" :disabled="isSearching" class="t-btn tiny cyan-border">
                {{ isSearching ? 'SCANNING...' : 'REFRESH_LIST' }}
            </button>
        </div>
    </div>

    <!-- ВИБІР КАМЕРИ -->
    <div class="selector-row">
        <div class="selector-container">
            <label class="hud-label">DEVICE_PATH</label>
            <div class="select-wrapper">
                <select v-model="selectedPath" @change="selectCamera" class="t-select" :disabled="isStreaming">
                    <option v-if="cameras.length === 0" value="">NO_DEVICES_FOUND</option>
                    <option v-for="cam in cameras" :key="cam.path" :value="cam.path">
                        {{ cam.name }}
                    </option>
                </select>
                <div class="select-arrow">▼</div>
            </div>
        </div>
        
        <button 
        @click="toggleStream" 
        class="t-btn power-btn" 
        :class="isStreaming ? 'red-border' : 'yellow-border'"
        >
        {{ isStreaming ? 'TERMINATE_FEED' : 'INITIATE_FEED' }}
        </button>
    </div>

    <!-- ВІДЕОПОТІК -->
    <div class="camera-viewport" :class="{ 'no-signal-border': !isStreaming }">
        <!-- Якщо стрімінг активний - показуємо картинку, інакше плейсхолдер -->
        <template v-if="isStreaming">
            <img :src="cameraUrl" alt="VIDEO_STREAM" class="video-feed" @error="handleImgError" />
            <audio v-if="isStreaming" :src="`http://${serverIP}:8081`" autoplay></audio>
        </template>
        
        <div v-else class="static-placeholder">
        <div class="scanline"></div>
        <div class="no-signal-text">SIGNAL_LOST_..._AWAITING_COMMAND</div>
        </div>

        <div class="corner-brackets"></div>
    </div>

    <!-- ІНФО-ПАНЕЛЬ -->
    <div class="camera-footer-meta">
        <div class="meta-item">
            <span class="label">ADDR:</span>
            <span class="value cyan">{{ serverIP }}:{{ camPort }}</span>
        </div>
        <div class="meta-item">
            <span class="label">STATUS:</span>
            <span class="value" :class="isStreaming ? 'yellow' : 'red'">{{ isStreaming ? 'ACTIVE' : 'IDLE' }}</span>
        </div>
    </div>
    </section>
</template>

<script setup>
import { ref, computed, onMounted, defineProps } from 'vue'

const props = defineProps({
    baseUrl: String,
    serverIP: String,
    camPort: { type: String, default: "8080" }
})

const cameras = ref([])
const selectedPath = ref('')
const isStreaming = ref(false)
const isSearching = ref(false)
const cameraKey = ref(0)

// Формуємо повний URL для MJPG потоку
const cameraUrl = computed(() => {
  return `http://${props.serverIP}:${props.camPort}/?action=stream&t=${cameraKey.value}`
})

const searchCameras = async () => {
  isSearching.value = true
  try {
    const res = await fetch(`${props.baseUrl}/api/camera?action=search`, { method: 'POST' })
    const data = await res.json()
    cameras.value = data
    if (data.length > 0 && !selectedPath.value) {
      selectedPath.value = data[0].path
      selectCamera()
    }
  } catch (e) { 
    console.error("[CAM] Search failed:", e) 
  } finally { 
    isSearching.value = false 
  }
}

const selectCamera = async () => {
  try {
    await fetch(`${props.baseUrl}/api/camera?action=select&path=${encodeURIComponent(selectedPath.value)}`, { method: 'POST' })
  } catch (e) { 
    console.error("[CAM] Selection failed:", e) 
  }
}

const toggleStream = async () => {
  const action = isStreaming.value ? 'stop' : 'start'
  try {
    const res = await fetch(`${props.baseUrl}/api/camera?action=${action}`, { method: 'POST' })
    if (res.ok) {
      if (action === 'start') {
        // Даємо серверу 1.5 сек на запуск заліза
        setTimeout(() => {
          isStreaming.value = true
          cameraKey.value++ // Оновлюємо ключ, щоб скинути кеш картинки
        }, 1500)
      } else {
        isStreaming.value = false
      }
    }
  } catch (e) { 
    console.error("[CAM] Toggle failed:", e) 
  }
}

const handleImgError = () => {
  console.error("[CAM] Image load error. Check if mjpg_streamer is running on port", props.camPort);
  isStreaming.value = false;
}

onMounted(() => {
  searchCameras()
})
</script>

<style scoped>
  @import "../assets/css/camera_module.css";
</style>