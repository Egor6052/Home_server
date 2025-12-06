<template>
  <div class="home-container">
    <div class="header">
      <h1>🌡️ Моніторинг Клімату</h1>
      <button @click="fetchData" :disabled="loading" class="refresh-btn">
        {{ loading ? 'Завантаження...' : 'Оновити дані' }}
      </button>
    </div>

    <div v-if="error" class="error-banner">
      {{ error }}
    </div>

    <div v-if="latestMeasurement" class="dashboard-grid">
      <div class="card temp-card">
        <div class="card-title">Температура</div>
        <div class="card-value">{{ formatNumber(latestMeasurement.temperature) }}°C</div>
        <div class="card-sub">Останнє оновлення</div>
      </div>

      <div class="card hum-card">
        <div class="card-title">Вологість</div>
        <div class="card-value">{{ formatNumber(latestMeasurement.humidity) }}%</div>
        <div class="card-sub">Стан повітря</div>
      </div>

      <div class="card info-card">
        <div class="card-title">Інфо станції</div>
        <div class="info-row">
          <span>ID:</span> <strong>{{ latestMeasurement.station_id }}</strong>
        </div>
        <div class="info-row">
          <span>Час:</span> <strong>{{ formatTime(latestMeasurement.timestamp) }}</strong>
        </div>
        <div class="info-row">
          <span>Коорд:</span> 
          <a 
            :href="`https://maps.google.com/?q=${latestMeasurement.lat},${latestMeasurement.lng}`" 
            target="_blank"
            class="map-link"
          >
            {{ latestMeasurement.lat }}, {{ latestMeasurement.lng }}
          </a>
        </div>
      </div>
    </div>

    <div class="chart-section" v-if="measurements.length > 0">
      <h2>Графік Температури</h2>
      <div class="chart-container">
        <LineChart :data="chartData" :options="chartOptions" />
      </div>
    </div>

    <div class="history-section">
      <h2>Історія вимірювань (Останні 10)</h2>
      <div class="table-responsive">
        <table>
          <thead>
            <tr>
              <th>Час</th>
              <th>Температура</th>
              <th>Вологість</th>
              <th>ID Станції</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in limitedHistory" :key="item.id">
              <td>{{ formatFullDate(item.timestamp) }}</td>
              <td :class="getTempClass(item.temperature)">
                {{ formatNumber(item.temperature) }}°C
              </td>
              <td>{{ formatNumber(item.humidity) }}%</td>
              <td>{{ item.station_id }}</td>
            </tr>
            <tr v-if="measurements.length === 0 && !loading">
              <td colspan="4" class="empty-state">Дані відсутні</td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>
  </div>
</template>

<script>
// Імпорти для Chart.js
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler // Для заливки під графіком
} from 'chart.js'
import { Line as LineChart } from 'vue-chartjs'

// Реєстрація компонентів графіка
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
  Filler
)

export default {
  name: 'HomeView',
  components: { LineChart }, // Реєструємо компонент в Vue
  data() {
    return {
      measurements: [],
      loading: false,
      error: null,
      apiUrl: "https://home-server-9e586-default-rtdb.firebaseio.com/measurements.json?auth=YOUR_AUTH_KEY",
      
      // Налаштування вигляду графіка
      chartOptions: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { display: false }, // Ховаємо легенду, бо лінія одна
          tooltip: {
            mode: 'index',
            intersect: false,
          }
        },
        scales: {
          y: {
            beginAtZero: false, // Графік фокусується на діапазоні температур
            grid: { color: '#e0e0e0' }
          },
          x: {
            grid: { display: false } // Прибираємо вертикальні лінії для чистоти
          }
        },
        elements: {
          line: {
            tension: 0.4 // Робить лінію плавною (хвилястою)
          }
        }
      }
    };
  },
  computed: {
    // Сортуємо: нові зверху (для логіки)
    sortedMeasurements() {
      return [...this.measurements].sort((a, b) => {
        // === ВИПРАВЛЕННЯ: ЗАМІНЮЄМО '_' НА 'T' ===
        // 1. Створюємо рядок, зрозумілий Date API (YYYY-MM-DDTHH:MM:SS)
        const dateStringA = b.timestamp.replace('_', 'T');
        const dateStringB = a.timestamp.replace('_', 'T');
        
        // 2. Порівнюємо нові об'єкти Date
        return new Date(dateStringA) - new Date(dateStringB);
      });
    },
    
    // НОВЕ: Обмеження для таблиці (Тільки 10 записів)
    limitedHistory() {
      return this.sortedMeasurements.slice(0, 10);
    },

    // Беремо найперший елемент
    latestMeasurement() {
      return this.sortedMeasurements.length > 0 ? this.sortedMeasurements[0] : null;
    },

    // НОВЕ: Підготовка даних для Графіка
    chartData() {
      // Для графіка беремо, наприклад, останні 20 точок, щоб було видно динаміку
      // Але графік має йти зліва направо (від старих до нових), тому reverse()
      const dataForChart = this.sortedMeasurements.slice(0, 20).reverse();

      return {
        labels: dataForChart.map(m => this.formatTime(m.timestamp)), // Вісь X (Час)
        datasets: [
          {
            label: '°C',
            backgroundColor: 'rgba(255, 99, 132, 0.2)', // Колір заливки (блідо-червоний)
            borderColor: 'rgba(255, 99, 132, 1)',       // Колір лінії (червоний)
            borderWidth: 2,
            pointRadius: 3, // Розмір точок
            fill: true,     // Заливка під графіком (ефект амплітуди)
            data: dataForChart.map(m => parseFloat(m.temperature)) // Вісь Y
          }
        ]
      }
    }
  },
  methods: {
    async fetchData() {
      this.loading = true;
      this.error = null;
      try {
        const response = await fetch(this.apiUrl);
        if (!response.ok) {
          throw new Error(`HTTP помилка! статус: ${response.status}`);
        }
        const data = await response.json();
        
        if (data) {
          this.measurements = Object.keys(data).map(key => ({
            id: key,
            ...data[key]
          }));
        } else {
          this.measurements = [];
        }
      } catch (err) {
        this.error = "Не вдалося завантажити дані: " + err.message;
        console.error(err);
      } finally {
        this.loading = false;
      }
    },
    formatNumber(val) {
      if (val === undefined || val === null || val === "") return "--";
      return parseFloat(val).toFixed(1);
    },
    formatTime(timestampString) {
      if (!timestampString) return "--:--";
      
      // Очікуваний формат: YYYY-MM-DD_HH:MM:SS. Беремо тільки частину після '_'
      const parts = timestampString.split('_');
      if (parts.length > 1) {
        // Повертаємо тільки години та хвилини
        return parts[1].substring(0, 5); 
      }
      return timestampString; // Повертаємо оригінал, якщо формат невірний
    },
    formatFullDate(timestampString) {
      if (!timestampString) return "Невідомо";
      
      // Формат: YYYY-MM-DD_HH:MM:SS
      // Просто замінюємо підкреслення на пробіл, щоб отримати чистий рядок: YYYY-MM-DD HH:MM:SS
      return timestampString.replace('_', ' ');
    },
    getTempClass(temp) {
      const t = parseFloat(temp);
      if (t < 10) return 'text-cold';
      if (t > 28) return 'text-hot';
      return 'text-normal';
    }
  },
  mounted() {
    this.fetchData();
    setInterval(this.fetchData, 60000);
  }
};
</script>

<style lang="less" scoped>
  @import "../src/assets/css/app.css";

</style>