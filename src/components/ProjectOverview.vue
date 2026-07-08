<template>
  <section>
    <h2>项目概述</h2>
    <p class="lead">
      <mark class="key">BackPet</mark> 灵感来源于 BongoCat 的 Windows 桌面互动组件再造，结合手机端小组件的便捷功能，为用户提供持续的情绪价值。它可以永远显示在桌面最上层，用可爱的互动陪伴你的工作和学习。
    </p>
    <p class="lead">
      不同于传统的桌面宠物，BackPet 支持多种功能组件自由切换，用户可以根据自己的喜好自定义外观、行为和功能模块，真正做到功能性和情感陪伴的结合。
    </p>

    <div class="demo-container">
      <h3>互动演示 · 点击试试</h3>
      <InteractivePet ref="petRef" @mood-change="handleMoodChange" />
      <div class="mood-display" :style="{ color: moodColor }">{{ moodText }}</div>
      <div class="controls">
        <button class="btn" @click="setMood('happy')">开心鼓掌</button>
        <button class="btn" @click="setMood('sleep')">犯困打盹</button>
        <button class="btn" @click="setMood('excited')">高兴呐喊</button>
        <button class="btn secondary" @click="resetMood">平静下来</button>
      </div>
    </div>

    <div class="demo-button-container">
      <button class="demo-btn" @click="$emit('navigate', 'demo')">
        <span class="demo-icon">🖥️</span>
        <span class="demo-text">体验测试</span>
        <span class="demo-arrow">→</span>
      </button>
      <p class="demo-hint">进入模拟Windows桌面体验互动组件（含时钟、天气、宠物）</p>
    </div>
  </section>
</template>

<script setup>
import { ref } from 'vue'
import InteractivePet from './InteractivePet.vue'

const petRef = ref(null)
const moodText = ref('😊 安静待机中')
const moodColor = ref('#fbbf24')

function handleMoodChange(mood) {
  moodText.value = mood.text
  moodColor.value = mood.color
}

function setMood(mood) {
  petRef.value?.setMood(mood)
}

function resetMood() {
  petRef.value?.resetMood()
}
</script>

<style scoped>
.demo-container {
  background: var(--bg2);
  border-radius: 16px;
  padding: 2rem;
  margin: 2rem 0;
  text-align: center;
}

.mood-display {
  font-size: 1.25rem;
  font-weight: 600;
  color: var(--accent);
  margin-top: 1rem;
}

.controls {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  justify-content: center;
  margin-top: 1.5rem;
}
.btn {
  padding: 0.75rem 1.5rem;
  border: none;
  border-radius: 25px;
  background: linear-gradient(135deg, var(--accent), var(--accent2));
  color: white;
  font-size: 0.875rem;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
}
.btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 5px 15px rgba(0, 0, 0, 0.2);
}
.btn.secondary {
  background: var(--bg2);
  color: var(--ink);
}

.demo-button-container {
  text-align: center;
  margin-top: 2rem;
  padding-top: 1.5rem;
  border-top: 2px dashed var(--rule);
}

.demo-btn {
  display: inline-flex;
  align-items: center;
  gap: 12px;
  padding: 16px 32px;
  border: none;
  border-radius: 16px;
  background: linear-gradient(135deg, #1a1a2e, #16213e);
  color: white;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.2);
}

.demo-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 15px 40px rgba(0, 0, 0, 0.3);
  background: linear-gradient(135deg, #252545, #202a4a);
}

.demo-btn:active {
  transform: translateY(-1px);
}

.demo-icon {
  font-size: 24px;
}

.demo-text {
  flex: 1;
}

.demo-arrow {
  font-size: 18px;
  transition: transform 0.3s ease;
}

.demo-btn:hover .demo-arrow {
  transform: translateX(5px);
}

.demo-hint {
  margin-top: 12px;
  font-size: 14px;
  color: var(--muted);
}
</style>