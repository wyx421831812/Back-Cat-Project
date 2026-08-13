#ifndef BONGOMODELMANAGER_H
#define BONGOMODELMANAGER_H

#include <QObject>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCoreApplication>

/**
 * @brief BongoCat 模型定义
 *
 * 支持两种模型格式:
 * 1. BongoCat 标准格式 (Live2D):
 *    modelDir/
 *      cat.moc3, cat.model3.json, cat.physics3.json
 *      cat.2048/texture_00.png          - Live2D纹理
 *      resources/
 *        cover.png                      - 猫咪底图
 *        background.png                 - 键盘布局背景
 *        left-keys/KeyA.png ...         - 按键图片
 *
 * 2. 简化格式:
 *    modelDir/
 *      cover.png         - 底图(必需)
 *      background.png    - 背景图(可选)
 *      keys/KeyA.png ... - 按键图片(可选)
 *      model.json        - 配置(可选)
 */
struct BongoModel {
    QString id;
    QString name;
    QString path;

    // 核心图片
    QPixmap coverImage;       // 猫咪底图
    QPixmap backgroundImage;  // 背景图(键盘布局)
    QPixmap leftHandDownImage;
    QPixmap rightHandDownImage;

    // 按键图片
    QMap<QString, QPixmap> keyImages;

    // Live2D相关(可选)
    QString live2dModelFile;  // .model3.json 路径
    QString live2dTextureFile; // 纹理文件路径

    bool isPreset = false;

    bool isValid() const {
        return !coverImage.isNull();
    }

    // 获取按键图片(优先从keyImages查找)
    QPixmap getKeyImage(const QString &keyName) const {
        auto it = keyImages.find(keyName);
        if (it != keyImages.end()) return it.value();
        return QPixmap();
    }
};

/**
 * @brief BongoCat 模型管理器
 *
 * 负责加载和管理BongoCat模型
 */
class BongoModelManager : public QObject
{
    Q_OBJECT

public:
    static BongoModelManager &instance();

    void loadModels();
    QList<BongoModel> models() const;
    BongoModel currentModel() const;
    QString currentModelId() const;
    bool setCurrentModel(const QString &id);
    BongoModel getModelById(const QString &id) const;

    // 从任意路径导入模型(自动识别格式)
    bool importModel(const QString &sourcePath, const QString &modelName);

    // 从BongoCat标准文件夹导入
    bool importBongoCatModel(const QString &sourcePath, const QString &modelName);

    // 删除自定义模型
    bool deleteModel(const QString &id);

    QString userModelsDirectory() const;
    static QStringList supportedKeys();

signals:
    void modelChanged(const QString &modelId);
    void modelsLoaded();

private:
    BongoModelManager(QObject *parent = nullptr);
    BongoModelManager(const BongoModelManager &) = delete;
    BongoModelManager &operator=(const BongoModelManager &) = delete;

    // 自动识别并加载模型
    BongoModel autoDetectAndLoadModel(const QString &dirPath, bool isPreset);

    // 从BongoCat标准结构加载
    BongoModel loadBongoCatFormat(const QString &dirPath);

    // 从简化格式加载
    BongoModel loadSimpleFormat(const QString &dirPath);

    // 从资源加载预设模型
    BongoModel loadPresetFromResources();

    // 加载按键图片 (left-keys/ 或 keys/)
    void loadKeyImages(const QString &keysDir, BongoModel &model);

    void loadUserModels();
    void ensureDirectories();
    void saveCurrentModel(const QString &id);
    QString loadCurrentModelId();

    QMap<QString, BongoModel> m_models;
    QString m_currentModelId;
};

#endif // BONGOMODELMANAGER_H
