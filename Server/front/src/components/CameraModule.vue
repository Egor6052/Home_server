<template>
  <section class="config-card camera-card">
    <!-- HEADER -->
    <div class="camera-header">
      <div class="title-group">
        <h2 class="section-title">OPTICAL_FEED_01</h2>
        <div class="status-indicator" :class="{ live: isStreaming }">
          <div class="dot"></div>
          <span>{{ isStreaming ? 'LIVE_STREAM' : 'STANDBY' }}</span>
        </div>
      </div>
    </div>

    <!-- ВІДЕОПОТІК -->
    <div class="camera-viewport" :class="{ 'no-signal-border': !isStreaming }">
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
        <span class="label">SRC:</span>
        <span class="value cyan">camera1.m3u8</span>
      </div>
      <div class="meta-item">
        <span class="label">STATUS:</span>
        <span class="value" :class="isStreaming ? 'yellow' : 'red'">{{ isStreaming ? 'ACTIVE' : 'IDLE' }}</span>
      </div>
    </div>
  </section>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount } from 'vue'
import Hls from 'hls.js'

// Фиксированная ссылка на поток — если адрес сменится, менять только тут.
const STREAM_URL = 'http://10.88.83.28/hls/camera1.m3u8'

const isStreaming = ref(false)
const videoEl = ref(null)
let hls = null

const startStream = () => {
  if (!videoEl.value) return

  if (Hls.isSupported()) {
    hls = new Hls()
    hls.loadSource(STREAM_URL)
    hls.attachMedia(videoEl.value)
    hls.on(Hls.Events.MANIFEST_PARSED, () => {
      videoEl.value.play().catch(() => { /* автоплей может быть заблокирован, видео muted */ })
    })
    hls.on(Hls.Events.ERROR, (_event, data) => {
      console.error('[CAM] HLS error:', data)
    })
  } else if (videoEl.value.canPlayType('application/vnd.apple.mpegurl')) {
    // Safari умеет HLS нативно, hls.js тут не нужен
    videoEl.value.src = STREAM_URL
    videoEl.value.play().catch(() => {})
  } else {
    console.error('[CAM] Этот браузер не поддерживает HLS-воспроизведение')
    return
  }

  isStreaming.value = true
}

const stopStream = () => {
  if (hls) {
    hls.destroy()
    hls = null
  }
  if (videoEl.value) {
    videoEl.value.pause()
    videoEl.value.removeAttribute('src')
    videoEl.value.load()
  }
  isStreaming.value = false
}

onMounted(startStream)
onBeforeUnmount(stopStream)
</script>

<style scoped>
  @import "../assets/css/camera_module.css";
</style>