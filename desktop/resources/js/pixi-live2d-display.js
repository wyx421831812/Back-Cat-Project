var __pow = Math.pow;
var __async = (__this, __arguments, generator) => {
  return new Promise((resolve, reject) => {
    var fulfilled = (value) => {
      try {
        step(generator.next(value));
      } catch (e) {
        reject(e);
      }
    };
    var rejected = (value) => {
      try {
        step(generator.throw(value));
      } catch (e) {
        reject(e);
      }
    };
    var step = (x) => x.done ? resolve(x.value) : Promise.resolve(x.value).then(fulfilled, rejected);
    step((generator = generator.apply(__this, __arguments)).next());
  });
};
(function(global, factory) {
  typeof exports === "object" && typeof module !== "undefined" ? factory(exports, require("@pixi/utils"), require("@pixi/math"), require("@pixi/core"), require("@pixi/display")) : typeof define === "function" && define.amd ? define(["exports", "@pixi/utils", "@pixi/math", "@pixi/core", "@pixi/display"], factory) : (global = typeof globalThis !== "undefined" ? globalThis : global || self, factory((global.PIXI = global.PIXI || {}, global.PIXI.live2d = global.PIXI.live2d || {}), global.PIXI.utils, global.PIXI, global.PIXI, global.PIXI));
})(this, function(exports2, utils, math, core, display) {
  "use strict";
  const LOGICAL_WIDTH = 2;
  const LOGICAL_HEIGHT = 2;
  var CubismConfig;
  ((CubismConfig2) => {
    CubismConfig2.supportMoreMaskDivisions = true;
    CubismConfig2.setOpacityFromMotion = false;
  })(CubismConfig || (CubismConfig = {}));
  exports2.config = void 0;
  ((config2) => {
    config2.LOG_LEVEL_VERBOSE = 0;
    config2.LOG_LEVEL_WARNING = 1;
    config2.LOG_LEVEL_ERROR = 2;
    config2.LOG_LEVEL_NONE = 999;
    config2.logLevel = config2.LOG_LEVEL_WARNING;
    config2.sound = true;
    config2.motionSync = true;
    config2.motionFadingDuration = 500;
    config2.idleMotionFadingDuration = 2e3;
    config2.expressionFadingDuration = 500;
    config2.preserveExpressionOnMotion = true;
    config2.cubism4 = CubismConfig;
  })(exports2.config || (exports2.config = {}));
  const VERSION = "0.4.0";
  const logger = {
    log(tag, ...messages) {
      if (exports2.config.logLevel <= exports2.config.LOG_LEVEL_VERBOSE) {
        console.log(`[${tag}]`, ...messages);
      }
    },
    warn(tag, ...messages) {
      if (exports2.config.logLevel <= exports2.config.LOG_LEVEL_WARNING) {
        console.warn(`[${tag}]`, ...messages);
      }
    },
    error(tag, ...messages) {
      if (exports2.config.logLevel <= exports2.config.LOG_LEVEL_ERROR) {
        console.error(`[${tag}]`, ...messages);
      }
    }
  };
  function clamp(num, lower, upper) {
    return num < lower ? lower : num > upper ? upper : num;
  }
  function rand(min, max) {
    return Math.random() * (max - min) + min;
  }
  function copyProperty(type, from, to, fromKey, toKey) {
    const value = from[fromKey];
    if (value !== null && typeof value === type) {
      to[toKey] = value;
    }
  }
  function copyArray(type, from, to, fromKey, toKey) {
    const array = from[fromKey];
    if (Array.isArray(array)) {
      to[toKey] = array.filter((item) => item !== null && typeof item === type);
    }
  }
  function applyMixins(derivedCtor, baseCtors) {
    baseCtors.forEach((baseCtor) => {
      Object.getOwnPropertyNames(baseCtor.prototype).forEach((name) => {
        if (name !== "constructor") {
          Object.defineProperty(derivedCtor.prototype, name, Object.getOwnPropertyDescriptor(baseCtor.prototype, name));
        }
      });
    });
  }
  function folderName(url) {
    let lastSlashIndex = url.lastIndexOf("/");
    if (lastSlashIndex != -1) {
      url = url.slice(0, lastSlashIndex);
    }
    lastSlashIndex = url.lastIndexOf("/");
    if (lastSlashIndex !== -1) {
      url = url.slice(lastSlashIndex + 1);
    }
    return url;
  }
  function remove(array, item) {
    const index = array.indexOf(item);
    if (index !== -1) {
      array.splice(index, 1);
    }
  }
  class ExpressionManager extends utils.EventEmitter {
    constructor(settings, options) {
      super();
      this.expressions = [];
      this.reserveExpressionIndex = -1;
      this.destroyed = false;
      this.settings = settings;
      this.tag = `ExpressionManager(${settings.name})`;
    }
    init() {
      this.defaultExpression = this.createExpression({}, void 0);
      this.currentExpression = this.defaultExpression;
      this.stopAllExpressions();
    }
    loadExpression(index) {
      return __async(this, null, function* () {
        if (!this.definitions[index]) {
          logger.warn(this.tag, `Undefined expression at [${index}]`);
          return void 0;
        }
        if (this.expressions[index] === null) {
          logger.warn(this.tag, `Cannot set expression at [${index}] because it's already failed in loading.`);
          return void 0;
        }
        if (this.expressions[index]) {
          return this.expressions[index];
        }
        const expression = yield this._loadExpression(index);
        this.expressions[index] = expression;
        return expression;
      });
    }
    _loadExpression(index) {
      throw new Error("Not implemented.");
    }
    setRandomExpression() {
      return __async(this, null, function* () {
        if (this.definitions.length) {
          const availableIndices = [];
          for (let i = 0; i < this.definitions.length; i++) {
            if (this.expressions[i] !== null && this.expressions[i] !== this.currentExpression && i !== this.reserveExpressionIndex) {
              availableIndices.push(i);
            }
          }
          if (availableIndices.length) {
            const index = Math.floor(Math.random() * availableIndices.length);
            return this.setExpression(index);
          }
        }
        return false;
      });
    }
    resetExpression() {
      this._setExpression(this.defaultExpression);
    }
    restoreExpression() {
      this._setExpression(this.currentExpression);
    }
    setExpression(index) {
      return __async(this, null, function* () {
        if (typeof index !== "number") {
          index = this.getExpressionIndex(index);
        }
        if (!(index > -1 && index < this.definitions.length)) {
          return false;
        }
        if (index === this.expressions.indexOf(this.currentExpression)) {
          return false;
        }
        this.reserveExpressionIndex = index;
        const expression = yield this.loadExpression(index);
        if (!expression || this.reserveExpressionIndex !== index) {
          return false;
        }
        this.reserveExpressionIndex = -1;
        this.currentExpression = expression;
        this._setExpression(expression);
        return true;
      });
    }
    update(model, now) {
      if (!this.isFinished()) {
        return this.updateParameters(model, now);
      }
      return false;
    }
    destroy() {
      this.destroyed = true;
      this.emit("destroy");
      const self2 = this;
      self2.definitions = void 0;
      self2.expressions = void 0;
    }
  }
  const EPSILON = 0.01;
  const MAX_SPEED = 40 / 7.5;
  const ACCELERATION_TIME = 1 / (0.15 * 1e3);
  class FocusController {
    constructor() {
      this.targetX = 0;
      this.targetY = 0;
      this.x = 0;
      this.y = 0;
      this.vx = 0;
      this.vy = 0;
    }
    focus(x, y, instant = false) {
      this.targetX = clamp(x, -1, 1);
      this.targetY = clamp(y, -1, 1);
      if (instant) {
        this.x = this.targetX;
        this.y = this.targetY;
      }
    }
    update(dt) {
      const dx = this.targetX - this.x;
      const dy = this.targetY - this.y;
      if (Math.abs(dx) < EPSILON && Math.abs(dy) < EPSILON)
        return;
      const d = Math.sqrt(__pow(dx, 2) + __pow(dy, 2));
      const maxSpeed = MAX_SPEED / (1e3 / dt);
      let ax = maxSpeed * (dx / d) - this.vx;
      let ay = maxSpeed * (dy / d) - this.vy;
      const a = Math.sqrt(__pow(ax, 2) + __pow(ay, 2));
      const maxA = maxSpeed * ACCELERATION_TIME * dt;
      if (a > maxA) {
        ax *= maxA / a;
        ay *= maxA / a;
      }
      this.vx += ax;
      this.vy += ay;
      const v = Math.sqrt(__pow(this.vx, 2) + __pow(this.vy, 2));
      const maxV = 0.5 * (Math.sqrt(__pow(maxA, 2) + 8 * maxA * d) - maxA);
      if (v > maxV) {
        this.vx *= maxV / v;
        this.vy *= maxV / v;
      }
      this.x += this.vx;
      this.y += this.vy;
    }
  }
  class ModelSettings {
    constructor(json) {
      this.json = json;
      let url2 = json.url;
      if (typeof url2 !== "string") {
        throw new TypeError("The `url` field in settings JSON must be defined as a string.");
      }
      this.url = url2;
      this.name = folderName(this.url);
    }
    resolveURL(path) {
      return utils.url.resolve(this.url, path);
    }
    replaceFiles(replacer) {
      this.moc = replacer(this.moc, "moc");
      if (this.pose !== void 0) {
        this.pose = replacer(this.pose, "pose");
      }
      if (this.physics !== void 0) {
        this.physics = replacer(this.physics, "physics");
      }
      for (let i = 0; i < this.textures.length; i++) {
        this.textures[i] = replacer(this.textures[i], `textures[${i}]`);
      }
    }
    getDefinedFiles() {
      const files = [];
      this.replaceFiles((file) => {
        files.push(file);
        return file;
      });
      return files;
    }
    validateFiles(files) {
      const assertFileExists = (expectedFile, shouldThrow) => {
        const actualPath = this.resolveURL(expectedFile);
        if (!files.includes(actualPath)) {
          if (shouldThrow) {
            throw new Error(`File "${expectedFile}" is defined in settings, but doesn't exist in given files`);
          }
          return false;
        }
        return true;
      };
      const essentialFiles = [this.moc, ...this.textures];
      essentialFiles.forEach((texture) => assertFileExists(texture, true));
      const definedFiles = this.getDefinedFiles();
      return definedFiles.filter((file) => assertFileExists(file, false));
    }
  }
  var MotionPriority = /* @__PURE__ */ ((MotionPriority2) => {
    MotionPriority2[MotionPriority2["NONE"] = 0] = "NONE";
    MotionPriority2[MotionPriority2["IDLE"] = 1] = "IDLE";
    MotionPriority2[MotionPriority2["NORMAL"] = 2] = "NORMAL";
    MotionPriority2[MotionPriority2["FORCE"] = 3] = "FORCE";
    return MotionPriority2;
  })(MotionPriority || {});
  class MotionState {
    constructor() {
      this.debug = false;
      this.currentPriority = 0;
      this.reservePriority = 0;
    }
    reserve(group, index, priority) {
      if (priority <= 0) {
        logger.log(this.tag, `Cannot start a motion with MotionPriority.NONE.`);
        return false;
      }
      if (group === this.currentGroup && index === this.currentIndex) {
        logger.log(this.tag, `Motion is already playing.`, this.dump(group, index));
        return false;
      }
      if (group === this.reservedGroup && index === this.reservedIndex || group === this.reservedIdleGroup && index === this.reservedIdleIndex) {
        logger.log(this.tag, `Motion is already reserved.`, this.dump(group, index));
        return false;
      }
      if (priority === 1) {
        if (this.currentPriority !== 0) {
          logger.log(this.tag, `Cannot start idle motion because another motion is playing.`, this.dump(group, index));
          return false;
        }
        if (this.reservedIdleGroup !== void 0) {
          logger.log(this.tag, `Cannot start idle motion because another idle motion has reserved.`, this.dump(group, index));
          return false;
        }
        this.setReservedIdle(group, index);
      } else {
        if (priority < 3) {
          if (priority <= this.currentPriority) {
            logger.log(this.tag, "Cannot start motion because another motion is playing as an equivalent or higher priority.", this.dump(group, index));
            return false;
          }
          if (priority <= this.reservePriority) {
            logger.log(this.tag, "Cannot start motion because another motion has reserved as an equivalent or higher priority.", this.dump(group, index));
            return false;
          }
        }
        this.setReserved(group, index, priority);
      }
      return true;
    }
    start(motion, group, index, priority) {
      if (priority === 1) {
        this.setReservedIdle(void 0, void 0);
        if (this.currentPriority !== 0) {
          logger.log(this.tag, "Cannot start idle motion because another motion is playing.", this.dump(group, index));
          return false;
        }
      } else {
        if (group !== this.reservedGroup || index !== this.reservedIndex) {
          logger.log(this.tag, "Cannot start motion because another motion has taken the place.", this.dump(group, index));
          return false;
        }
        this.setReserved(void 0, void 0, 0);
      }
      if (!motion) {
        return false;
      }
      this.setCurrent(group, index, priority);
      return true;
    }
    complete() {
      this.setCurrent(void 0, void 0, 0);
    }
    setCurrent(group, index, priority) {
      this.currentPriority = priority;
      this.currentGroup = group;
      this.currentIndex = index;
    }
    setReserved(group, index, priority) {
      this.reservePriority = priority;
      this.reservedGroup = group;
      this.reservedIndex = index;
    }
    setReservedIdle(group, index) {
      this.reservedIdleGroup = group;
      this.reservedIdleIndex = index;
    }
    isActive(group, index) {
      return group === this.currentGroup && index === this.currentIndex || group === this.reservedGroup && index === this.reservedIndex || group === this.reservedIdleGroup && index === this.reservedIdleIndex;
    }
    reset() {
      this.setCurrent(void 0, void 0, 0);
      this.setReserved(void 0, void 0, 0);
      this.setReservedIdle(void 0, void 0);
    }
    shouldRequestIdleMotion() {
      return this.currentGroup === void 0 && this.reservedIdleGroup === void 0;
    }
    shouldOverrideExpression() {
      return !exports2.config.preserveExpressionOnMotion && this.currentPriority > 1;
    }
    dump(requestedGroup, requestedIndex) {
      if (this.debug) {
        const keys = [
          "currentPriority",
          "reservePriority",
          "currentGroup",
          "currentIndex",
          "reservedGroup",
          "reservedIndex",
          "reservedIdleGroup",
          "reservedIdleIndex"
        ];
        return `
<Requested> group = "${requestedGroup}", index = ${requestedIndex}
` + keys.map((key) => "[" + key + "] " + this[key]).join("\n");
      }
      return "";
    }
  }
  const TAG$2 = "SoundManager";
  const VOLUME = 0.5;
  class SoundManager {
    static get volume() {
      return this._volume;
    }
    static set volume(value) {
      this._volume = (value > 1 ? 1 : value < 0 ? 0 : value) || 0;
      this.audios.forEach((audio) => audio.volume = this._volume);
    }
    static add(file, onFinish, onError) {
      const audio = new Audio(file);
      audio.volume = this._volume;
      audio.preload = "auto";
      audio.addEventListener("ended", () => {
        this.dispose(audio);
        onFinish == null ? void 0 : onFinish();
      });
      audio.addEventListener("error", (e) => {
        this.dispose(audio);
        logger.warn(TAG$2, `Error occurred on "${file}"`, e.error);
        onError == null ? void 0 : onError(e.error);
      });
      this.audios.push(audio);
      return audio;
    }
    static play(audio) {
      return new Promise((resolve, reject) => {
        var _a;
        (_a = audio.play()) == null ? void 0 : _a.catch((e) => {
          audio.dispatchEvent(new ErrorEvent("error", { error: e }));
          reject(e);
        });
        if (audio.readyState === audio.HAVE_ENOUGH_DATA) {
          resolve();
        } else {
          audio.addEventListener("canplaythrough", resolve);
        }
      });
    }
    static dispose(audio) {
      audio.pause();
      audio.removeAttribute("src");
