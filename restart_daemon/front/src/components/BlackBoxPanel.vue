<template>
  <div class="blackbox-panel">
    <div class="bb-settings-row">
      <div class="field-group">
        <label for="bb-path">Log directory</label>
        <input
          id="bb-path"
          v-model="newPath"
          type="text"
          placeholder="/path/to/logs або /dev/null"
          :disabled="loadingPath"
        />
        <span class="hint">Поточна: {{ currentPath || (loadingPath ? '...' : 'unknown') }}</span>
      </div>
      <button class="btn btn-primary" :disabled="loadingPath || saving" @click="handleSave">
        {{ saving ? 'Saving...' : 'Save' }}
      </button>
    </div>

    <p v-if="statusMessage" class="status-line" :class="isError ? 'is-error' : 'is-success'">
      {{ statusMessage }}
    </p>

    <div class="bb-body">
      <div class="bb-files-list">
        <div class="bb-files-head">
          <span>Файли ({{ files.length }})</span>
          <button @click="loadFileList" :disabled="filesLoading">
            {{ filesLoading ? '...' : 'Reload' }}
          </button>
        </div>

        <p v-if="filesError" class="bb-files-empty" style="color: var(--accent-red)">{{ filesError }}</p>
        <ul v-else class="bb-files-ul">
          <li v-if="files.length === 0" class="bb-files-empty">No log files</li>
          <li
            v-for="file in sortedFiles"
            :key="file"
            class="bb-file-item"
            :class="{ active: file === selectedFile }"
            @click="selectFile(file)"
          >
            <span class="bb-file-name">{{ file }}</span>
            <a
              class="icon-btn"
              title="Завантажити"
              :href="downloadUrl(file)"
              @click.stop
            >&#8595;</a>
            <button
              class="icon-btn danger"
              title="Видалити"
              @click.stop="confirmDelete(file)"
            >&#10005;</button>
          </li>
        </ul>
      </div>

      <div class="bb-content-panel">
        <div class="bb-content-head">{{ selectedFile || 'File not selected' }}</div>
        <p v-if="contentError" class="bb-content-placeholder" style="color: var(--accent-red)">{{ contentError }}</p>
        <p v-else-if="contentLoading" class="bb-content-placeholder">Loading...</p>
        <p v-else-if="!selectedFile" class="bb-content-placeholder">Select a file from the list on the left</p>
        <pre v-else class="bb-content-body">{{ fileContent }}</pre>
      </div>
    </div>
  </div>
</template>

<script>
import {
  getBlackBoxSettings,
  setBlackBoxSettings,
  listBlackBoxFiles,
  getBlackBoxFileContent,
  removeBlackBoxFile,
  blackBoxDownloadUrl
} from '../api.js';

export default {
  name: 'BlackBoxPanel',
  data() {
    return {
      currentPath: '',
      newPath: '',
      loadingPath: true,
      saving: false,
      statusMessage: '',
      isError: false,

      files: [],
      filesLoading: false,
      filesError: '',

      selectedFile: '',
      fileContent: '',
      contentLoading: false,
      contentError: ''
    };
  },

  computed: {
    // Від найстарішого файлу (найменша дата) до найновішого (найбільша дата)
    sortedFiles() {
      return [...this.files].sort((a, b) => {
        const dateA = this.extractDateFromFilename(a);
        const dateB = this.extractDateFromFilename(b);

        if (dateA === null && dateB === null) return a.localeCompare(b);
        if (dateA === null) return 1;  // без розпізнаної дати - в кінець
        if (dateB === null) return -1;

        return dateA - dateB;
      });
    }
  },
  mounted() {
    this.loadCurrentPath();
    this.loadFileList();
  },
  methods: {
    extractDateFromFilename(filename) {
      // Формат: black_box_YYYY-MM-DD.log
      const match = filename.match(/(\d{4})-(\d{2})-(\d{2})/);
      if (!match) return null;

      const [, year, month, day] = match;
      const ts = new Date(`${year}-${month}-${day}T00:00:00`).getTime();
      return isNaN(ts) ? null : ts;
    },
    async loadCurrentPath() {
      this.loadingPath = true;
      try {
        const data = await getBlackBoxSettings();
        this.currentPath = data.path || '';
        this.newPath = this.currentPath;
      } catch (e) {
        this.statusMessage = 'Failed to load current path.';
        this.isError = true;
      } finally {
        this.loadingPath = false;
      }
    },

    async handleSave() {
      if (!this.newPath || !this.newPath.trim()) {
        this.statusMessage = 'Path cannot be empty.';
        this.isError = true;
        return;
      }

      this.saving = true;
      this.statusMessage = '';

      try {
        const data = await setBlackBoxSettings(this.newPath.trim());
        if (data.status === 'success') {
          this.statusMessage = 'Saved. Log path updated.';
          this.isError = false;
          this.currentPath = this.newPath.trim();
        } else {
          this.statusMessage = data.message || 'Failed to save path';
          this.isError = true;
        }
      } catch (e) {
        this.statusMessage = 'Network error while saving';
        this.isError = true;
      } finally {
        this.saving = false;
      }
    },

    async loadFileList() {
      this.filesLoading = true;
      this.filesError = '';
      try {
        this.files = await listBlackBoxFiles();
      } catch (e) {
        this.filesError = 'Failed to load file list.';
      } finally {
        this.filesLoading = false;
      }
    },

    async selectFile(filename) {
      this.selectedFile = filename;
      this.fileContent = '';
      this.contentError = '';
      this.contentLoading = true;

      try {
        this.fileContent = await getBlackBoxFileContent(filename);
      } catch (e) {
        this.contentError = e.message || 'Failed to load file contents.';
      } finally {
        this.contentLoading = false;
      }
    },

    confirmDelete(filename) {
      if (window.confirm(`Remove "${filename}"? The action cannot be undone.`)) {
        this.deleteFile(filename);
      }
    },

    downloadUrl(filename) {
      return blackBoxDownloadUrl(filename);
    },

    async deleteFile(filename) {
      try {
        const data = await removeBlackBoxFile(filename);
        if (data.status === 'success') {
          this.files = this.files.filter((f) => f !== filename);
          if (this.selectedFile === filename) {
            this.selectedFile = '';
            this.fileContent = '';
            this.contentError = '';
          }
        } else {
          this.filesError = data.message || 'Could not delete file.';
        }
      } catch (e) {
        this.filesError = 'Network error while deleting.';
      }
    }
  }
};
</script>


<style scoped>
  @import "../assets/css/blackbox.css";
</style>
