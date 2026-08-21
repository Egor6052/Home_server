<template>
  <div class="view-page">
    <div class="bb-view-header">
      <button class="btn" @click="goBack" title="Назад до Admin Panel">&larr; Назад</button>
      <h1 class="section-label" style="flex: 1; margin: 0;">BlackBox log</h1>
    </div>

    <BlackBoxPanel />
  </div>
</template>

<script>
import BlackBoxPanel from '../components/BlackBoxPanel.vue';
export default {
  name: 'BlackBoxSettingsView',
  components: { BlackBoxPanel },
  methods: {
    goBack() {
      this.$router.push({ name: 'telemetry' });
    }
  }
};
</script>

<style scoped>
  .bb-view-header {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-bottom: 20px;
  }
</style>