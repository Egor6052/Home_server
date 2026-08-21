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
        :disabled="isBusy"
        class="t-btn power-btn"
        :class="isStreaming ? 'red-border' : 'yellow-border'"
        >
        {{ isStreaming ? 'TERMINATE_FEED' : 'INITIATE_FEED' }}
        </button>
    </div>

    <!-- ВІДЕОПОТІК -->
    <div class="camera-viewport" :class="{ 'no-signal-border': !isStreaming }">
        <!-- WebSocket + MSE (mpegts.js) замінює старий MJPG <img>.
             Елемент лишається в DOM завжди, щоб не пере-створювати його —
             плеєр підключається/відключається окремо через watch(isStreaming). -->
        <video v-show="isStreaming" ref="videoEl" class="video-feed" muted autoplay playsinline></video>

        <div v-if="!isStreaming" class="static-placeholder">
        <div class="scanline"></div>
        <div class="no-signal-text">SIGNAL_LOST_..._AWAITING_COMMAND</div>
        </div>

        <div class="corner-brackets"></div>
    </div>

    <!-- ІНФО-ПАНЕЛЬ -->
    <div class="camera-footer-meta">
        <div class="meta-item">
            <span class="label">ADDR:</span>
            <span class="value cyan">{{ serverIP }}:{{ wsPort }}</span>
        </div>
        <div class="meta-item">
            <span class="label">STATUS:</span>
            <span class="value" :class="isStreaming ? 'yellow' : 'red'">{{ isStreaming ? 'ACTIVE' : 'IDLE' }}</span>
        </div>
    </div>
    </section>
</template>

<script setup>
import { ref, computed, onMounted, onBeforeUnmount, watch, defineProps } from 'vue'
import mpegts from 'mpegts.js'

const props = defineProps({
    baseUrl: String,
    serverIP: String,
    wsPort: { type: [String, Number], default: 9002 }, // порт VideoStream (WebSocket)
    cameraId: { type: String, default: 'camera1' }       // шлях ws://.../<cameraId>
})

const cameras = ref([])
const selectedPath = ref('')
const isStreaming = ref(false)
const isSearching = ref(false)
const isBusy = ref(false) // блокуємо кнопку старт/стоп на час запиту, щоб не наспамити

const videoEl = ref(null)
let player = null
let eventSource = null

const videoUrl = computed(() => `http://${props.serverIP}:${props.wsPort}/${props.cameraId}`)

// ---- REST-дії ----

// Знімок поточного стану — викликається один раз при завантаженні сторінки,
// щоб не залежати від того, застала вона попередні SSE-події чи ні
// (наприклад, якщо камеру вже запустив інший користувач раніше).
const fetchStatus = async () => {
    try {
        const res = await fetch(`${props.baseUrl}/api/camera?action=status`)
        const data = await res.json()
        selectedPath.value = data.selected_path || ''
        isStreaming.value = !!data.is_streaming
    } catch (e) {
        console.error('[CAM] Status fetch failed:', e)
    }
}

const searchCameras = async () => {
    isSearching.value = true
    try {
        const res = await fetch(`${props.baseUrl}/api/camera?action=search`, { method: 'POST' })
        const data = await res.json()
        cameras.value = data
        // Автовибір лише якщо камеру ще ніхто не обрав (в т.ч. на іншій сторінці)
        if (data.length > 0 && !selectedPath.value) {
            selectedPath.value = data[0].path
            await selectCamera()
        }
    } catch (e) {
        console.error('[CAM] Search failed:', e)
    } finally {
        isSearching.value = false
    }
}

const selectCamera = async () => {
    try {
        await fetch(`${props.baseUrl}/api/camera?action=select&path=${encodeURIComponent(selectedPath.value)}`, { method: 'POST' })
    } catch (e) {
        console.error('[CAM] Selection failed:', e)
    }
}

const toggleStream = async () => {
    const action = isStreaming.value ? 'stop' : 'start'
    isBusy.value = true
    try {
        // isStreaming свідомо не змінюємо тут локально: справжній стан прийде
        // через /api/events і однаково застосується на всіх відкритих
        // сторінках, включно з цією — так усі бачать один і той самий стан.
        await fetch(`${props.baseUrl}/api/camera?action=${action}`, { method: 'POST' })
    } catch (e) {
        console.error('[CAM] Toggle failed:', e)
    } finally {
        isBusy.value = false
    }
}

// ---- Відео: WebSocket + MSE (mpegts.js) ----

const attachPlayer = () => {
    if (!videoEl.value || !mpegts.isSupported()) {
        console.error('[CAM] MSE playback not supported in this browser')
        return
    }
    detachPlayer()

    player = mpegts.createPlayer({ type: 'mse', isLive: true, url: videoUrl.value })
    player.on(mpegts.Events.ERROR, (type, detail) => {
        console.error('[CAM] Player error:', type, detail)
    })
    player.attachMediaElement(videoEl.value)
    player.load()
    player.play().catch(() => { /* автоплей може бути заблокований, відео все одно muted */ })
}

const detachPlayer = () => {
    if (!player) return
    try {
        player.pause()
        player.unload()
        player.detachMediaElement()
        player.destroy()
    } catch (e) { /* нема сенсу зупиняти вже мертвий плеєр */ }
    player = null
}

// Плеєр реагує на isStreaming незалежно від того, хто саме натиснув кнопку —
// це і є синхронізація: подія від сервера однаково запускає/гасить відео
// на всіх відкритих сторінках.
watch(isStreaming, (streaming) => {
    if (streaming) attachPlayer()
    else detachPlayer()
})

// ---- Синхронізація стану між клієнтами (SSE) ----

const connectEvents = () => {
    eventSource = new EventSource(`${props.baseUrl}/api/events`)
    eventSource.onmessage = (e) => {
        const { type, data } = JSON.parse(e.data)
        if (type === 'cameras_found') {
            cameras.value = data
        } else if (type === 'camera_selected') {
            selectedPath.value = data.path
        } else if (type === 'camera_state') {
            isStreaming.value = data.status === 'started'
        }
    }
    eventSource.onerror = () => {
        // EventSource сам перепідключається; просто лишаємо слід у консолі
        console.warn('[CAM] SSE connection lost, browser will retry automatically')
    }
}

onMounted(async () => {
    connectEvents()
    await fetchStatus()
    await searchCameras()
    if (isStreaming.value) attachPlayer()
})

onBeforeUnmount(() => {
    detachPlayer()
    eventSource?.close()
})
</script>

<style scoped>
  @import "../assets/css/camera_module.css";
</style>