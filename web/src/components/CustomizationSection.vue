<template>
  <section>
    <h2>自定义功能</h2>
    <p>MoodPet 真正做到了<mark class="key">多种定义</mark>，用户可以自由定制这些属性：</p>

    <div class="customization-options">
      <div>
        <h3>主题配色</h3>
        <div class="customization-options">
          <div class="color-option cyan">
            <div class="color-preview"></div>
            <span>青蓝渐变</span>
          </div>
          <div class="color-option pink">
            <div class="color-preview"></div>
            <span>粉紫渐变</span>
          </div>
          <div class="color-option green">
            <div class="color-preview"></div>
            <span>翠绿渐变</span>
          </div>
          <div class="color-option orange">
            <div class="color-preview"></div>
            <span>橙红渐变</span>
          </div>
          <div class="color-option yellow-purple">
            <div class="color-preview"></div>
            <span>黄紫渐变</span>
          </div>
        </div>
      </div>
      <div>
        <h3>自定义背景主题</h3>
        <ul style="margin-left: 1.5rem; color: var(--muted);">
          <li>纯色背景</li>
          <li>渐变背景</li>
          <li>自定义图片背景</li>
          <li>透明背景</li>
          <li>毛玻璃效果</li>
        </ul>
        <h3 style="margin-top: 1.5rem;">其他调节选项</h3>
        <ul style="margin-left: 1.5rem; color: var(--muted);">
          <li>窗口大小 & 位置</li>
          <li>背景透明度</li>
          <li>动画频率</li>
          <li>点击响应灵敏度</li>
          <li>是否置顶显示</li>
          <li>是否启用点击穿透</li>
        </ul>
      </div>
    </div>
  </section>
</template>

<script setup>
</script>

<style scoped>
.customization-options {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 1rem;
  margin-top: 1.5rem;
}
.color-option {
  padding: 1rem;
  border-radius: 8px;
  border: 2px solid var(--rule);
  text-align: center;
  cursor: pointer;
  transition: all 0.3s ease;
}
.color-option:hover {
  border-color: var(--accent);
}
.color-preview {
  width: 50px;
  height: 50px;
  border-radius: 50%;
  margin: 0 auto 0.5rem;
}
.color-option.cyan .color-preview { background: linear-gradient(135deg, #06b6d4, #8b5cf6); }
.color-option.pink .color-preview { background: linear-gradient(135deg, #ec4899, #8b5cf6); }
.color-option.green .color-preview { background: linear-gradient(135deg, #10b981, #06b6d4); }
.color-option.orange .color-preview { background: linear-gradient(135deg, #f59e0b, #ef4444); }
.color-option.yellow-purple .color-preview { background: linear-gradient(135deg, #fbbf24, #8b5cf6); }
</style>