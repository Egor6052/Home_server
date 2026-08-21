<template>
  <header class="header-container">
    <nav class="nav-bar">
      <div class="nav-group left">
        <div class="brand-section">
          <div class="brand">
            <span class="yellow-box"></span>
            <span class="brand-text">Home Server</span>
          </div>
        </div>
      </div>

      <div class="nav-group center">
        <!-- Тут ваші лінки -->
      </div>

      <div class="nav-group right">
        <!-- Кнопка відкриття панелі управління -->
        <button class="system-panel-btn" @click="toggleSidebar">
          SYSTEM PANEL
        </button>

        <div class="system-status">
          <div class="live-indicator">
            <span class="pulse"></span>
            <span class="live-text">LIVE_FEED</span>
          </div>
          <div class="status-divider"></div>
        </div>
      </div>
    </nav>

    <!-- Універсальне бокове меню -->
    <div :class="['system-sidebar', { 'active': isSidebarOpen }]">
      <div class="sidebar-header">
        <h3>System Control</h3>
        <button @click="isSidebarOpen = false" class="close-btn">✕</button>
      </div>

      <!-- Блок швидких дій -->
      <div class="sidebar-actions">
        <button 
          class="action-btn restart-btn" 
          :disabled="isRestarting" 
          @click="restartDaemon"
        >
          <span v-if="!isRestarting">Restart Daemon</span>
          <span v-else>Restarting...</span>
        </button>
        
        <button class="action-btn refresh-btn" @click="fetchLogs">
          Update Logs
        </button>
      </div>

      <div class="logs-section">
        <div class="logs-label">System Logs:</div>
        <div class="logs-content" ref="logsContainer">
          <div v-for="(log, index) in logs" :key="index" :class="['log-line', getLogClass(log)]">
            {{ log }}
          </div>
          <div v-if="logs.length === 0" class="no-logs">No logs available...</div>
        </div>
      </div>
    </div>
    
    <div v-if="isSidebarOpen" class="overlay" @click="isSidebarOpen = false"></div>
  </header>
</template>

<script setup>
import { ref, onMounted } from 'vue'

const isSidebarOpen = ref(false)
const isRestarting = ref(false)
const logs = ref([])
const logsContainer = ref(null)

// --- ФУНКЦІЇ ЛОГІВ ---
async function fetchLogs() {
  try {
    const response = await fetch('/api/log') 
    const data = await response.json()
    logs.value = data.logs
    
    setTimeout(() => {
      if (logsContainer.value) {
        logsContainer.value.scrollTop = logsContainer.value.scrollHeight
      }
    }, 100)
  } catch (error) {
    console.error('Failed to fetch logs:', error)
  }
}


// --- ФУНКЦІЯ РЕСТАРТУ (Оновлена) ---
async function restartDaemon() {
  // Залишаємо лише одне підтвердження перед дією
  if (!confirm("Restart the daemon? Connection will be lost for a few seconds.")) {
    return;
  }

  isRestarting.value = true;
  
  try {
    const response = await fetch('/api/restart', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' }
    });
    
    if (response.ok) {
      // Замість alert просто закриваємо сайдбар. 
      // Користувач зрозуміє, що команда пішла, по тому, як панель зникне.
      isSidebarOpen.value = false;
      console.log("Restart command sent successfully");
    }
  } catch (error) {
    console.error('Restart failed:', error);
    // Alert на помилку краще залишити, щоб розуміти, чому не спрацювало
    alert("System error: Could not reach the server.");
  } finally {
    isRestarting.value = false;
  }
}



function toggleSidebar() {
  isSidebarOpen.value = !isSidebarOpen.value
  if (isSidebarOpen.value) {
    fetchLogs()
  }
}

function getLogClass(log) {
  if (log.includes('[ERROR]')) return 'log-error'
  if (log.includes('[Web]')) return 'log-info'
  if (log.includes('[UART]')) return 'log-uart'
  return ''
}

onMounted(() => {
  setInterval(() => {
    if (isSidebarOpen.value) fetchLogs()
  }, 5000)
})
</script>

<style scoped>
  @import "../assets/css/app_header.css";
</style>