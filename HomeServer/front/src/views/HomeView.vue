<template>
  <div class="data-view-container">
    <DataPage />
  </div>
</template>

<script>
import DataPage from '../components/DataPage.vue'

export default {
    name: 'DataView',
    components: {
        // Реєструємо його
        DataPage
    }
}
</script>

<style scoped>

.data-view-container {
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