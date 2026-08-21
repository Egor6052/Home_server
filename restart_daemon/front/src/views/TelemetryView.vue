<template>
  <div class="telemetry-view-container">
    <TelemetryPage />
  </div>
</template>

<script>
import TelemetryPage from '../components/TelemetryPage.vue'

export default {
  name: 'TelemetryView',
  components: {
    TelemetryPage
  }
}
</script>

<style scoped>
.telemetry-view-container {
  background: #0d0d0d;
  min-height: 100vh;
  width: 100%;
  display: flex;
  flex-direction: column;
}

:global(body) {
  margin: 0;
  padding: 0;
  background-color: #0d0d0d;
}
</style>