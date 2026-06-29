<template>
  <Header v-if="showHeader" />
  <router-view />
</template>

<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import Header from '@/components/AppHeader.vue'

const route = useRoute()

const hiddenRoutes = ['/login', '/change-password']

const showHeader = computed(() => {
  return !hiddenRoutes.includes(route.path)
})
</script>

<style>
  @import '@/assets/css/app.css';
</style>