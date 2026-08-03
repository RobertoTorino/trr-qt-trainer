#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>

#include <QtGui/QIcon>
#include <QtGui/QDesktopServices>
#include <QtGui/QKeySequence>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtGui/QShortcut>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>

#include "app_logger.h"
#include "build_info.h"
#include "rpcs3_session_controller.h"
#include "ui_main_window.h"
#include "ui_runtime_dialog.h"
#include "ui_value_writes_dialog.h"

namespace
{
constexpr std::uint64_t kBase = 0x300000000ULL;
constexpr std::uint64_t kThisPointerAddr = 0x3200D26BCULL;
constexpr int kPointerRetryCount = 30;
constexpr int kPointerRetryDelayMs = 200;
constexpr int kVerifyRetryCount = 8;
constexpr int kVerifyRetryDelayMs = 40;
constexpr int kStabilizeCycles = 40;
constexpr int kLockIntervalMs = 250;
constexpr int kConnectionStatusIntervalMs = 5000;
constexpr int kModeResetPulseMs = 120;
constexpr std::uint32_t kRoundTimerTicksPerSecond = 60;

constexpr std::uint32_t kOffP1Id = 0x170;
constexpr std::uint32_t kOffP2Id = 0x174;
constexpr std::uint32_t kOffGameState = 0x178;
constexpr std::uint32_t kOffCounterP1 = 0x290;
constexpr std::uint32_t kOffCounterP2 = 0x29C;
constexpr std::uint32_t kOffRoundTimer = 0x2A0;
constexpr std::uint32_t kOffUiFlags = 0x2AC;
constexpr std::uint32_t kOffInfiniteRound = 0x2B4;
constexpr std::uint32_t kOffStageId = 0x2B8;

constexpr std::uint64_t kAddrP1State = kBase + 0x12DA338ULL;
constexpr std::uint64_t kAddrP2State = kBase + 0x12DC7D8ULL;
constexpr std::uint64_t kAddrP1PosX = kBase + 0x12D9F00ULL;
constexpr std::uint64_t kAddrP1PosY = kBase + 0x12D9F04ULL;
constexpr std::uint64_t kAddrP1PosZ = kBase + 0x12D9F08ULL;
constexpr std::uint64_t kAddrP1AnimationSpeed = kBase + 0x12DA59CULL;
constexpr std::uint64_t kAddrP2PosX = kBase + 0x12DC3A0ULL;
constexpr std::uint64_t kAddrP2PosY = kBase + 0x12DC3A4ULL;
constexpr std::uint64_t kAddrP2PosZ = kBase + 0x12DC3A8ULL;
constexpr std::uint64_t kAddrGameState = kBase + 0x12E9194ULL;
constexpr std::uint64_t kAddrGameStateRead = kBase + 0x2013F5B8ULL;
constexpr std::uint64_t kAddrGlobalStageId = kBase + 0x200D703CULL;

struct IdLabel
{
    const char* label;
    std::uint32_t value;
};

struct ModePreset
{
    std::uint32_t value;
    const char* label;
};

struct RoundTimePreset
{
    const char* label;
    std::optional<std::uint32_t> infiniteValue;
    std::optional<std::uint32_t> timerSeconds;
};

const std::array<IdLabel, 36> kCharacters{{
    {"Paul Phoenix", 0x000},
    {"Marshall Law", 0x00A},
    {"King", 0x015},
    {"Nina Williams", 0x020},
    {"Hwoarang", 0x02A},
    {"Ling Xiaoyu", 0x034},
    {"Christie Monteiro", 0x041},
    {"Eddy Gordo (softlocks)", 0x04B},
    {"Jin Kazama", 0x04C},
    {"Julia Chang", 0x056},
    {"Kuma", 0x05D},
    {"Bryan Fury", 0x064},
    {"Heihachi Mishima", 0x06E},
    {"Kazuya Mishima", 0x06F},
    {"Lee Chaolan", 0x079},
    {"Steve Fox", 0x07D},
    {"Mokujin", 0x088},
    {"Jack-6", 0x08B},
    {"Asuka Kazama", 0x099},
    {"Devil Jin", 0x0A8},
    {"Feng Wei", 0x0B3},
    {"Armor King", 0x0BA},
    {"Lili", 0x0BE},
    {"Sergei Dragunov", 0x0CB},
    {"Bob", 0x0D5},
    {"Zafina (softlocks)", 0x0D9},
    {"Miguel", 0x0DA},
    {"Leo", 0x0E1},
    {"Lars", 0x0EB},
    {"Alisa", 0x0F5},
    {"Jinpachi", 0x102},
    {"Ogre", 0x103},
    {"Jun", 0x105},
    {"Kinjin", 0x10E},
    {"Eliza", 0x126},
    {"Eliza v2", 0x12C},
}};

const std::array<IdLabel, 19> kStages{{
    {"02 Eternal Paradise", 0x02},
    {"03 Historic Town Square", 0x03},
    {"04 Condor Canyon", 0x04},
    {"05 Arctic Dream", 0x05},
    {"08 Moonlit Wilderness", 0x08},
    {"0B Sakura Schoolyard", 0x0B},
    {"0C Tempest", 0x0C},
    {"0D Winter Palace", 0x0D},
    {"0E Hall of Judgement", 0x0E},
    {"0F Naraku", 0x0F},
    {"18 Darkness", 0x18},
    {"22 Practice | Walls", 0x22},
    {"23 Practice | No Walls", 0x23},
    {"28 Fireworks Over Barcelona", 0x28},
    {"2A Riverside Promenade", 0x2A},
    {"2B Tropical Rainforest", 0x2B},
    {"2C Moai Excavation", 0x2C},
    {"2D Extravagant Underground", 0x2D},
    {"2E Tulip Festival", 0x2E},
}};

const std::array<ModePreset, 5> kModePresets{{
    {1, "1 Versus | Continuous Fight"},
    {2, "2 Interactive Splash Demo (0x2)"},
    {3, "3 Unknown (0x3)"},
    {4, "4 Round Reset | Wake Workaround"},
    {5, "5 Practice Mode | Stable"},
}};

const std::array<RoundTimePreset, 5> kRoundTimePresets{{
    {"Infinite Round Time", 1U, std::nullopt},
    {"30 Seconds", 0U, 30U},
    {"60 Seconds", 0U, 60U},
    {"90 Seconds", 0U, 90U},
    {"Custom", std::nullopt, std::nullopt},
}};

bool readMemory(HANDLE process, std::uint64_t addr, void* out, std::size_t size)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(addr), out, size, &bytesRead) && bytesRead == size;
}

bool writeMemory(HANDLE process, std::uint64_t addr, const void* in, std::size_t size)
{
    SIZE_T bytesWritten = 0;
    return WriteProcessMemory(process, reinterpret_cast<LPVOID>(addr), in, size, &bytesWritten) && bytesWritten == size;
}

std::optional<std::uint32_t> readBE32(HANDLE process, std::uint64_t addr)
{
    std::array<std::uint8_t, 4> b{};
    if (!readMemory(process, addr, b.data(), b.size()))
    {
        return std::nullopt;
    }

    return (static_cast<std::uint32_t>(b[0]) << 24U) |
           (static_cast<std::uint32_t>(b[1]) << 16U) |
           (static_cast<std::uint32_t>(b[2]) << 8U) |
           static_cast<std::uint32_t>(b[3]);
}

bool writeBE32(HANDLE process, std::uint64_t addr, std::uint32_t value)
{
    const std::array<std::uint8_t, 4> b{
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(value & 0xFFU),
    };
    return writeMemory(process, addr, b.data(), b.size());
}

std::optional<float> readBEFloat(HANDLE process, std::uint64_t addr)
{
    const auto bits = readBE32(process, addr);
    if (!bits.has_value())
    {
        return std::nullopt;
    }

    float value = 0.0F;
    const std::uint32_t raw = bits.value();
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

std::uint32_t floatBits(float value)
{
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

std::optional<std::uint64_t> resolveBattlePtr(HANDLE process)
{
    const auto ptr = readBE32(process, kThisPointerAddr);
    if (!ptr.has_value() || ptr.value() == 0)
    {
        return std::nullopt;
    }
    return kBase + ptr.value();
}

QString formatHex64(std::uint64_t value)
{
    return QStringLiteral("0x%1").arg(QString::number(static_cast<qulonglong>(value), 16).toUpper());
}

QString formatHex32(std::uint32_t value)
{
    return QStringLiteral("0x%1").arg(QString::number(value, 16).rightJustified(8, QChar('0')).toUpper());
}

QIcon makeIndicatorIcon(const QColor& fillColor, const QColor& borderColor)
{
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(borderColor, 1.3));
    painter.setBrush(fillColor);
    painter.drawEllipse(QRectF(1.5, 1.5, 11.0, 11.0));
    painter.end();

    return QIcon(pixmap);
}

QIcon makeLeftAccentIcon(const QColor& fillColor)
{
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fillColor);
    painter.drawRoundedRect(QRectF(2.0, 2.0, 4.0, 10.0), 2.0, 2.0);
    painter.end();

    return QIcon(pixmap);
}

void applyWindowFrameTone(HWND hwnd)
{
    if (hwnd == nullptr)
    {
        return;
    }

    const BOOL enabled = TRUE;
    const COLORREF frameColor = RGB(32, 32, 32);
    const COLORREF textColor = RGB(240, 240, 240);

    const DWORD darkModeAttr = 20;
    DwmSetWindowAttribute(hwnd, darkModeAttr, &enabled, sizeof(enabled));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &frameColor, sizeof(frameColor));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &frameColor, sizeof(frameColor));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &textColor, sizeof(textColor));
}

struct EnumWindowByPidData
{
    DWORD pid = 0;
    HWND hwnd = nullptr;
};

BOOL CALLBACK findWindowByPidProc(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<EnumWindowByPidData*>(lParam);
    if (data == nullptr || !IsWindowVisible(hwnd))
    {
        return TRUE;
    }

    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != data->pid)
    {
        return TRUE;
    }

    data->hwnd = hwnd;
    return FALSE;
}

HWND findTopLevelWindowByPid(DWORD pid)
{
    EnumWindowByPidData data{};
    data.pid = pid;
    EnumWindows(findWindowByPidProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

std::optional<std::uint32_t> parseU32Input(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        return std::nullopt;
    }

    bool ok = false;
    quint64 value = 0;
    if (trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
    {
        value = trimmed.mid(2).toULongLong(&ok, 16);
    }
    else
    {
        value = trimmed.toULongLong(&ok, 10);
    }

    if (!ok || value > 0xFFFFFFFFULL)
    {
        return std::nullopt;
    }

    return static_cast<std::uint32_t>(value);
}
}

class MainWindow final : public QMainWindow
{
public:
    MainWindow()
    {
        AppLogger::initialize();
        Ui::MainWindow mainUi;
        mainUi.setupUi(this);
        mainUi.statusPanelsLayout->setAlignment(Qt::AlignTop);
        const QString buildNumber = QString::number(BuildInfo::number).rightJustified(5, QLatin1Char('0'));
        setWindowTitle(QStringLiteral("TRR Qt Trainer %1-%2-%3 | %4 %5")
                   .arg(QString::fromLatin1(BuildInfo::version),
                    buildNumber,
                    QString::fromLatin1(BuildInfo::commitSha),
                    QString::fromLatin1(BuildInfo::channel),
                    QString::fromLatin1(BuildInfo::branch)));

        attachButton_ = mainUi.attachButton;
        applyButton_ = mainUi.applyButton;
        startRpcs3Button_ = mainUi.startRpcs3Button;
        startGameButton_ = mainUi.startGameButton;
        restartGameButton_ = mainUi.restartGameButton;
        resetEmulatorButton_ = mainUi.resetEmulatorButton;
        terminateRpcs3Button_ = mainUi.terminateRpcs3Button;
        rpcs3ConfigButton_ = mainUi.rpcs3ConfigButton;
        snapshotButton_ = mainUi.snapshotButton;
        trManualButton_ = mainUi.trManualButton;
        e3Button_ = mainUi.e3Button;
        showLogsButton_ = mainUi.showLogsButton;
        tutorialButton_ = mainUi.tutorialButton;
        refreshButton_ = mainUi.refreshButton;
        readLiveButton_ = mainUi.readLiveButton;
        stopLockButton_ = mainUi.stopLockButton;
        savePresetButton_ = mainUi.savePresetButton;
        loadPresetButton_ = mainUi.loadPresetButton;
        profileConservativeButton_ = mainUi.profileConservativeButton;
        profileBalancedButton_ = mainUi.profileBalancedButton;
        profileAggressiveButton_ = mainUi.profileAggressiveButton;
        runtimeButton_ = mainUi.runtimeButton;
        valueWritesButton_ = mainUi.valueWritesButton;
        advancedMemoryButton_ = mainUi.advancedMemoryButton;

        statusLabel_ = mainUi.statusLabel;
        pointerLabel_ = mainUi.pointerLabel;
        connectionStatusLabel_ = mainUi.connectionStatusLabel;
        runningGameLabel_ = mainUi.runningGameLabel;
        runningBuildLabel_ = mainUi.runningBuildLabel;
        p1Combo_ = mainUi.p1Combo;
        p2Combo_ = mainUi.p2Combo;
        stageCombo_ = mainUi.stageCombo;
        monitorP1Id_ = mainUi.monitorP1Id;
        monitorP2Id_ = mainUi.monitorP2Id;
        monitorStage_ = mainUi.monitorStage;
        monitorState_ = mainUi.monitorState;
        monitorTimer_ = mainUi.monitorTimer;
        monitorCounters_ = mainUi.monitorCounters;
        monitorUi_ = mainUi.monitorUi;
        monitorInf_ = mainUi.monitorInf;
        monitorGuard_ = mainUi.monitorGuard;

        std::array<QPushButton*, 25> topButtons{{
            attachButton_,
            applyButton_,
            startRpcs3Button_,
            mainUi.startCeButton,
            startGameButton_,
            restartGameButton_,
            resetEmulatorButton_,
            terminateRpcs3Button_,
            rpcs3ConfigButton_,
            snapshotButton_,
            trManualButton_,
            e3Button_,
            showLogsButton_,
            tutorialButton_,
            refreshButton_,
            readLiveButton_,
            stopLockButton_,
            savePresetButton_,
            loadPresetButton_,
            profileConservativeButton_,
            profileBalancedButton_,
            profileAggressiveButton_,
            runtimeButton_,
            valueWritesButton_,
            advancedMemoryButton_}};

        applyButton_->setIcon(makeLeftAccentIcon(QColor(46, 125, 50)));
        applyButton_->setIconSize(QSize(14, 14));
        e3Button_->setIcon(makeLeftAccentIcon(QColor(66, 133, 244)));
        e3Button_->setIconSize(QSize(14, 14));
        snapshotButton_->setIcon(makeLeftAccentIcon(QColor(239, 108, 0)));
        snapshotButton_->setIconSize(QSize(14, 14));
        terminateRpcs3Button_->setIcon(makeLeftAccentIcon(QColor(198, 40, 40)));
        terminateRpcs3Button_->setIconSize(QSize(14, 14));
        rpcs3ConfigButton_->setIcon(makeLeftAccentIcon(QColor(123, 31, 162)));
        rpcs3ConfigButton_->setIconSize(QSize(14, 14));
        showLogsButton_->setIcon(makeLeftAccentIcon(QColor(251, 192, 45)));
        showLogsButton_->setIconSize(QSize(14, 14));
        stopLockButton_->setIcon(makeLeftAccentIcon(QColor(255, 255, 255)));
        stopLockButton_->setIconSize(QSize(14, 14));
        trManualButton_->setIcon(makeLeftAccentIcon(QColor(124, 252, 0)));
        trManualButton_->setIconSize(QSize(14, 14));
        tutorialButton_->setIcon(makeLeftAccentIcon(QColor(233, 30, 99)));
        tutorialButton_->setIconSize(QSize(14, 14));
        startRpcs3Button_->setIcon(makeLeftAccentIcon(QColor(0, 255, 255)));
        startRpcs3Button_->setIconSize(QSize(14, 14));
        mainUi.startCeButton->setIcon(makeLeftAccentIcon(QColor(13, 94, 165)));
        mainUi.startCeButton->setIconSize(QSize(14, 14));
        startGameButton_->setIcon(makeLeftAccentIcon(QColor(0, 0, 128)));
        startGameButton_->setIconSize(QSize(14, 14));
        restartGameButton_->setIcon(makeLeftAccentIcon(QColor(255, 127, 80)));
        restartGameButton_->setIconSize(QSize(14, 14));
        resetEmulatorButton_->setIcon(makeLeftAccentIcon(QColor(0, 128, 128)));
        resetEmulatorButton_->setIconSize(QSize(14, 14));
        refreshButton_->setIcon(makeLeftAccentIcon(QColor(138, 43, 226)));
        refreshButton_->setIconSize(QSize(14, 14));
        readLiveButton_->setIcon(makeLeftAccentIcon(QColor(128, 128, 128)));
        readLiveButton_->setIconSize(QSize(14, 14));
        savePresetButton_->setIcon(makeLeftAccentIcon(QColor(165, 42, 42)));
        savePresetButton_->setIconSize(QSize(14, 14));
        loadPresetButton_->setIcon(makeLeftAccentIcon(QColor(96, 125, 139)));
        loadPresetButton_->setIconSize(QSize(14, 14));
        profileConservativeButton_->setIcon(makeLeftAccentIcon(QColor(63, 81, 181)));
        profileConservativeButton_->setIconSize(QSize(14, 14));
        profileBalancedButton_->setIcon(makeLeftAccentIcon(QColor(255, 193, 7)));
        profileBalancedButton_->setIconSize(QSize(14, 14));
        profileAggressiveButton_->setIcon(makeLeftAccentIcon(QColor(128, 0, 0)));
        profileAggressiveButton_->setIconSize(QSize(14, 14));
        advancedMemoryButton_->setIcon(makeLeftAccentIcon(QColor(219, 200, 166)));
        advancedMemoryButton_->setIconSize(QSize(14, 14));
        runtimeButton_->setIcon(makeLeftAccentIcon(QColor(17, 63, 139)));
        runtimeButton_->setIconSize(QSize(14, 14));
        valueWritesButton_->setIcon(makeLeftAccentIcon(QColor(66, 133, 244)));
        valueWritesButton_->setIconSize(QSize(14, 14));

        int maxButtonWidth = 0;
        int maxButtonHeight = 0;
        for (QPushButton* button : topButtons)
        {
            if (button == nullptr)
            {
                continue;
            }

            const QSize hint = button->sizeHint();
            maxButtonWidth = std::max(maxButtonWidth, hint.width());
            maxButtonHeight = std::max(maxButtonHeight, hint.height());
        }

        for (QPushButton* button : topButtons)
        {
            if (button == nullptr)
            {
                continue;
            }
            button->setFixedSize(maxButtonWidth, maxButtonHeight);
        }

        const int twoButtonColumnsWidth = (maxButtonWidth * 2) + mainUi.buttonGrid->horizontalSpacing();
        mainUi.connectionBox->setFixedWidth(twoButtonColumnsWidth);

        for (const auto& c : kCharacters)
        {
            const auto caption = QStringLiteral("%1 (0x%2)").arg(c.label).arg(QString::number(c.value, 16).rightJustified(3, QChar('0')).toUpper());
            p1Combo_->addItem(caption, static_cast<quint32>(c.value));
            p2Combo_->addItem(caption, static_cast<quint32>(c.value));
        }
        for (const auto& s : kStages)
        {
            stageCombo_->addItem(QStringLiteral("%1 (0x%2)").arg(s.label).arg(QString::number(s.value, 16).rightJustified(2, QChar('0')).toUpper()), static_cast<quint32>(s.value));
        }

        p1Combo_->setCurrentIndex(22); // Lili
        p2Combo_->setCurrentIndex(24); // Bob
        {
            const int defaultStageIdx = stageCombo_->findData(static_cast<quint32>(0x02));
            if (defaultStageIdx >= 0)
            {
                stageCombo_->setCurrentIndex(defaultStageIdx); // 02 Eternal Paradise
            }
        }

        runtimeDialog_ = new QDialog(this);
        Ui::RuntimeDialog runtimeUi;
        runtimeUi.setupUi(runtimeDialog_);
        lockCheckbox_ = runtimeUi.lockCheckbox;
        lockSelectionCheckbox_ = runtimeUi.lockSelectionCheckbox;
        autoDisableStageLockCheckbox_ = runtimeUi.autoDisableStageLockCheckbox;
        guardPauseCheckbox_ = runtimeUi.guardPauseCheckbox;
        guardPauseMsSpin_ = runtimeUi.guardPauseMsSpin;
        stabilizeCheckbox_ = runtimeUi.stabilizeCheckbox;
        modeResetPulseCheckbox_ = runtimeUi.modeResetPulseCheckbox;
        p1ControllerCheckbox_ = runtimeUi.p1ControllerCheckbox;
        p2CpuCheckbox_ = runtimeUi.p2CpuCheckbox;
        countersCheckbox_ = runtimeUi.countersCheckbox;
        p1CounterSpin_ = runtimeUi.p1CounterSpin;
        p2CounterSpin_ = runtimeUi.p2CounterSpin;

        valueWritesDialog_ = new QDialog(this);
        Ui::ValueWritesDialog valueWritesUi;
        valueWritesUi.setupUi(valueWritesDialog_);
        writeModeCheckbox_ = valueWritesUi.writeModeCheckbox;
        modePresetCombo_ = valueWritesUi.modePresetCombo;
        writeHpCheckbox_ = valueWritesUi.writeHpCheckbox;
        hpPresetCheckbox_ = valueWritesUi.hpPresetCheckbox;
        hpPresetCombo_ = valueWritesUi.hpPresetCombo;
        hpRandomCheckbox_ = valueWritesUi.hpRandomCheckbox;
        hpEdit_ = valueWritesUi.hpEdit;
        writeInfiniteRoundCheckbox_ = valueWritesUi.writeInfiniteRoundCheckbox;
        infiniteRoundSpin_ = valueWritesUi.infiniteRoundSpin;
        roundTimerCheckbox_ = valueWritesUi.roundTimerCheckbox;
        roundTimerSecondsSpin_ = valueWritesUi.roundTimerSecondsSpin;
        roundTimePresetCombo_ = valueWritesUi.roundTimePresetCombo;

        for (const auto& mode : kModePresets)
        {
            modePresetCombo_->addItem(QString::fromLatin1(mode.label), static_cast<quint32>(mode.value));
        }
        for (std::uint32_t value = 0; value <= 0x9000000U; value += 0x1000000U)
        {
            hpPresetCombo_->addItem(QStringLiteral("0x%1").arg(QString::number(value, 16).rightJustified(7, QChar('0')).toUpper()), static_cast<quint32>(value));
        }
        hpPresetCombo_->addItem(QStringLiteral("0x9999999"), static_cast<quint32>(0x9999999U));
        for (const auto& preset : kRoundTimePresets)
        {
            roundTimePresetCombo_->addItem(QString::fromLatin1(preset.label));
        }

        writeModeCheckbox_->setChecked(true);
        modePresetCombo_->setCurrentIndex(0);
        writeHpCheckbox_->setChecked(true);
        hpPresetCheckbox_->setChecked(true);
        writeInfiniteRoundCheckbox_->setChecked(true);
        infiniteRoundSpin_->setValue(1);
        p1ControllerCheckbox_->setChecked(true);
        p2CpuCheckbox_->setChecked(true);
        lockCheckbox_->setChecked(true);
        autoDisableStageLockCheckbox_->setChecked(true);
        guardPauseCheckbox_->setChecked(true);
        stabilizeCheckbox_->setChecked(true);
        roundTimePresetCombo_->setCurrentIndex(0);

        const auto applyToggleStyle = [](QCheckBox* box) {
            if (box == nullptr)
            {
                return;
            }

            if (box->isChecked())
            {
                box->setStyleSheet(QStringLiteral("QCheckBox { color: #4FA8FF; font-weight: 600; }"));
            }
            else
            {
                box->setStyleSheet(QString());
            }
        };

        std::vector<QCheckBox*> toggleBoxes{
            lockCheckbox_,
            lockSelectionCheckbox_,
            autoDisableStageLockCheckbox_,
            guardPauseCheckbox_,
            stabilizeCheckbox_,
            modeResetPulseCheckbox_,
            writeModeCheckbox_,
            hpPresetCheckbox_,
            hpRandomCheckbox_,
            writeHpCheckbox_,
            writeInfiniteRoundCheckbox_,
            roundTimerCheckbox_,
            p1ControllerCheckbox_,
            p2CpuCheckbox_,
            countersCheckbox_};

        auto* hpSourceGroup = new QButtonGroup(this);
        hpSourceGroup->setExclusive(true);
        hpSourceGroup->addButton(hpPresetCheckbox_);
        hpSourceGroup->addButton(hpRandomCheckbox_);

        for (QCheckBox* box : toggleBoxes)
        {
            applyToggleStyle(box);
            connect(box, &QCheckBox::toggled, this, [box, applyToggleStyle](bool) {
                applyToggleStyle(box);
            });
        }

        // Keep Attach RPCS3 native so the platform theme controls its appearance.

        loadRpcs3PathSetting();
        defaultPresetDirectory();

        timer_ = new QTimer(this);
        timer_->setInterval(kLockIntervalMs);
        connectionStatusTimer_ = new QTimer(this);
        connectionStatusTimer_->setInterval(kConnectionStatusIntervalMs);
        networkManager_ = new QNetworkAccessManager(this);

        connect(attachButton_, &QPushButton::clicked, this, [this](bool checked) {
            if (checked)
            {
                if (!attach())
                {
                    attachButton_->setChecked(false);
                    attachButton_->setText(QStringLiteral("Attach RPCS3"));
                }
            }
            else
            {
                detach();
            }
        });
        connect(startRpcs3Button_, &QPushButton::clicked, this, [this]() { showStartRpcs3Dialog(); });
        connect(mainUi.startCeButton, &QPushButton::clicked, this, [this]() { showStartCheatEngineDialog(); });
        connect(startGameButton_, &QPushButton::clicked, this, [this]() { showStartGameDialog(); });
        connect(restartGameButton_, &QPushButton::clicked, this, [this]() { restartConfiguredGame(); });
        connect(resetEmulatorButton_, &QPushButton::clicked, this, [this]() { resetEmulator(); });
        connect(terminateRpcs3Button_, &QPushButton::clicked, this, [this]() { terminateRpcs3(); });
        connect(rpcs3ConfigButton_, &QPushButton::clicked, this, [this]() { showRpcs3ConfigDialog(); });
        connect(snapshotButton_, &QPushButton::clicked, this, [this]() { takeSnapshot(); });
        connect(trManualButton_, &QPushButton::clicked, this, [this]() { openTrManual(); });
        connect(e3Button_, &QPushButton::clicked, this, [this]() { openE32013(); });
        connect(showLogsButton_, &QPushButton::clicked, this, [this]() { showLogs(); });
        connect(tutorialButton_, &QPushButton::clicked, this, [this]() { openBuildTutorial(); });
        connect(refreshButton_, &QPushButton::clicked, this, [this]() { refreshPointer(); });
        connect(applyButton_, &QPushButton::clicked, this, [this]() {
            stageLockArmed_ = true;
            applySelection();
        });
        connect(readLiveButton_, &QPushButton::clicked, this, [this]() { readLiveValuesIntoUi(); });
        connect(advancedMemoryButton_, &QPushButton::clicked, this, [this]() { showAdvancedMemoryDialog(); });
        connect(runtimeButton_, &QPushButton::clicked, this, [this]() { showRuntimeDialog(); });
        connect(valueWritesButton_, &QPushButton::clicked, this, [this]() { showValueWritesDialog(); });
        connect(stopLockButton_, &QPushButton::clicked, this, [this]() { stopRuntimeWrites(true); });
        connect(savePresetButton_, &QPushButton::clicked, this, [this]() { savePreset(); });
        connect(loadPresetButton_, &QPushButton::clicked, this, [this]() { loadPreset(); });
        connect(profileConservativeButton_, &QPushButton::clicked, this, [this]() { applyStabilityProfile(0); });
        connect(profileBalancedButton_, &QPushButton::clicked, this, [this]() { applyStabilityProfile(1); });
        connect(profileAggressiveButton_, &QPushButton::clicked, this, [this]() { applyStabilityProfile(2); });
        connect(stageCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { stageLockArmed_ = true; });

        connect(modePresetCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
            Q_UNUSED(idx);
        });

        connect(roundTimePresetCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
            if (idx < 0 || idx >= static_cast<int>(kRoundTimePresets.size()))
            {
                return;
            }
            const auto& preset = kRoundTimePresets[static_cast<std::size_t>(idx)];
            if (preset.infiniteValue.has_value())
            {
                infiniteRoundSpin_->setValue(static_cast<int>(preset.infiniteValue.value()));
            }
            if (preset.timerSeconds.has_value())
            {
                roundTimerCheckbox_->setChecked(true);
                roundTimerSecondsSpin_->setValue(static_cast<int>(preset.timerSeconds.value()));
            }
            else if (preset.infiniteValue.has_value() && preset.infiniteValue.value() == 1)
            {
                roundTimerCheckbox_->setChecked(false);
            }
        });

        connect(timer_, &QTimer::timeout, this, [this]() {
            updateRoundTransitionGuard();

            if (runtimeLockEnabled_)
            {
                if (autoDisableStageLockCheckbox_->isChecked() && stageLockArmed_ && shouldDisableStageLock())
                {
                    stageLockArmed_ = false;
                    statusLabel_->setText(QStringLiteral("Status: stage lock auto-disabled after match start"));
                }

                if (guardPauseCheckbox_->isChecked() && guardTicksRemaining_ > 0)
                {
                    --guardTicksRemaining_;
                    monitorGuard_->setText(QStringLiteral("Guard: paused writes (%1 ticks left)").arg(guardTicksRemaining_));
                }
                else
                {
                    monitorGuard_->setText(QStringLiteral("Guard: active"));
                    const bool writeSelection = lockSelectionCheckbox_->isChecked();
                    const bool writeStage = writeSelection && stageLockArmed_;
                    writeValuesFast(writeSelection, writeStage, true);
                }
            }
            else
            {
                monitorGuard_->setText(QStringLiteral("Guard: Lock Disabled"));
            }

            if (runtimeStabilizeCyclesLeft_ > 0)
            {
                if (!(guardPauseCheckbox_->isChecked() && guardTicksRemaining_ > 0))
                {
                    writeValuesFast(true, true, true);
                    --runtimeStabilizeCyclesLeft_;
                }
            }

            updateLiveMonitor();
        });

        connect(connectionStatusTimer_, &QTimer::timeout, this, [this]() {
            checkInternetConnection();
        });

        timer_->start();
        connectionStatusTimer_->start();
        checkInternetConnection();
        updateLiveMonitor();

        QTimer::singleShot(0, this, [this]() {
            applyWindowFrameTone(reinterpret_cast<HWND>(winId()));
        });
    }

    ~MainWindow() override
    {
        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }
    }

private:
    static QString pathDiagnostic(const QString& name, const QString& path)
    {
        const QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
        {
            return QStringLiteral("%1=<empty>").arg(name);
        }

        const QFileInfo info(trimmedPath);
        const QString canonicalPath = info.canonicalFilePath();
        return QStringLiteral("%1='%2' exists=%3 file=%4 canonical='%5'")
            .arg(name,
                 QDir::toNativeSeparators(trimmedPath),
                 info.exists() ? QStringLiteral("yes") : QStringLiteral("no"),
                 info.isFile() ? QStringLiteral("yes") : QStringLiteral("no"),
                 canonicalPath.isEmpty() ? QStringLiteral("<unresolved>") : QDir::toNativeSeparators(canonicalPath));
    }

    QString activeRpcs3ExePath() const
    {
        if (rpcs3ActiveExeChoice_ == 1)
        {
            return rpcs3LatestExePath_;
        }
        if (rpcs3ActiveExeChoice_ == 2)
        {
            return rpcs3CustomExePath_;
        }
        return rpcs3ExePath_;
    }

    QString activeRpcs3Label() const
    {
        return rpcs3BuildForChoice(rpcs3ActiveExeChoice_);
    }

    QString rpcs3ExePathForChoice(int choice) const
    {
        if (choice == 1)
        {
            return rpcs3LatestExePath_;
        }
        if (choice == 2)
        {
            return rpcs3CustomExePath_;
        }
        return rpcs3ExePath_;
    }

    QString rpcs3LabelForChoice(int choice) const
    {
        return rpcs3BuildForChoice(choice);
    }

    QString rpcs3BuildForChoice(int choice) const
    {
        if (choice == 1)
        {
            return rpcs3LatestBuild_;
        }
        if (choice == 2)
        {
            return rpcs3CustomBuild_;
        }
        return rpcs3Build_;
    }

    int rpcs3ChoiceForTitle(const QString& titleId) const
    {
        if (titleId.compare(QStringLiteral("NPUB31250"), Qt::CaseInsensitive) == 0)
        {
            return npub31250Rpcs3Choice_;
        }
        if (titleId.compare(QStringLiteral("NPJB00404"), Qt::CaseInsensitive) == 0)
        {
            return npjb00404Rpcs3Choice_;
        }
        return npeb01406Rpcs3Choice_;
    }

    QString rpcs3ConfigurationPath() const
    {
        return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config/rpcs3_config.json"));
    }

    void loadRpcs3PathSetting()
    {
        const QString configurationPath = rpcs3ConfigurationPath();
        QFile configurationFile(configurationPath);
        if (configurationFile.open(QIODevice::ReadOnly))
        {
            QJsonParseError parseError{};
            const QJsonDocument document = QJsonDocument::fromJson(configurationFile.readAll(), &parseError);
            configurationFile.close();
            if (parseError.error == QJsonParseError::NoError && document.isObject())
            {
                const QJsonObject root = document.object();
                const QJsonObject emulators = root.value(QStringLiteral("emulators")).toObject();
                const QJsonObject games = root.value(QStringLiteral("games")).toObject();
                const auto loadEmulator = [&emulators](const QString& key,
                                                       const QString& fallbackBuild,
                                                       QString& path,
                                                       QString& build) {
                    const QJsonValue value = emulators.value(key);
                    if (value.isObject())
                    {
                        const QJsonObject emulator = value.toObject();
                        path = QDir::toNativeSeparators(emulator.value(QStringLiteral("path")).toString().trimmed());
                        build = emulator.value(QStringLiteral("build")).toString(fallbackBuild).trimmed();
                    }
                    else
                    {
                        path = QDir::toNativeSeparators(value.toString().trimmed());
                        build = fallbackBuild;
                    }
                };
                loadEmulator(QStringLiteral("0.0.13"), QStringLiteral("0.0.13"), rpcs3ExePath_, rpcs3Build_);
                loadEmulator(QStringLiteral("latest"), QStringLiteral("0.0.00"), rpcs3LatestExePath_, rpcs3LatestBuild_);
                loadEmulator(QStringLiteral("custom"), QStringLiteral("0.0.00"), rpcs3CustomExePath_, rpcs3CustomBuild_);
                const QString legacyCheatEnginePath = root.value(QStringLiteral("cheatEnginePath")).toString().trimmed();
                cheatEngine72Path_ = QDir::toNativeSeparators(
                    root.value(QStringLiteral("cheatEngine72Path")).toString(legacyCheatEnginePath).trimmed());
                cheatEngine75Path_ = QDir::toNativeSeparators(
                    root.value(QStringLiteral("cheatEngine75Path")).toString().trimmed());
                rpcs3ActiveExeChoice_ = std::clamp(root.value(QStringLiteral("defaultEmulator")).toInt(0), 0, 2);

                const auto loadGame = [&games](const QString& titleId, QString& gamePath, int& emulatorChoice, int defaultChoice) {
                    const QJsonObject game = games.value(titleId).toObject();
                    gamePath = QDir::toNativeSeparators(game.value(QStringLiteral("bootPath")).toString().trimmed());
                    emulatorChoice = std::clamp(game.value(QStringLiteral("emulator")).toInt(defaultChoice), 0, 2);
                };
                loadGame(QStringLiteral("NPEB01406"), npeb01406GamePath_, npeb01406Rpcs3Choice_, 0);
                loadGame(QStringLiteral("NPUB31250"), npub31250GamePath_, npub31250Rpcs3Choice_, 1);
                loadGame(QStringLiteral("NPJB00404"), npjb00404GamePath_, npjb00404Rpcs3Choice_, 1);

                refreshActiveGamePath();
                syncRpcs3SessionPaths();
                AppLogger::info(QStringLiteral("Loaded current RPCS3 configuration file '%1': default=%2; mappings=NPEB01406:%3, NPUB31250:%4, NPJB00404:%5")
                                    .arg(QDir::toNativeSeparators(configurationPath),
                                         activeRpcs3Label(),
                                         rpcs3LabelForChoice(npeb01406Rpcs3Choice_),
                                         rpcs3LabelForChoice(npub31250Rpcs3Choice_),
                                         rpcs3LabelForChoice(npjb00404Rpcs3Choice_)));
                return;
            }

            AppLogger::error(QStringLiteral("Invalid RPCS3 configuration file '%1': %2. Falling back to legacy settings.")
                                 .arg(QDir::toNativeSeparators(configurationPath), parseError.errorString()));
        }

        // One-time migration for installations that predate rpcs3_config.json.
        QSettings settings(QStringLiteral("TekkenRevolutionReborn"), QStringLiteral("TRRQtTrainer"));
        rpcs3ExePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("rpcs3ExePath")).toString().trimmed());
        rpcs3LatestExePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("rpcs3LatestExePath")).toString().trimmed());
        rpcs3CustomExePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("rpcs3CustomExePath")).toString().trimmed());
        cheatEngine72Path_ = QDir::toNativeSeparators(settings.value(QStringLiteral("cheatEnginePath")).toString().trimmed());
        cheatEngine75Path_ = QDir::toNativeSeparators(settings.value(QStringLiteral("cheatEngine75Path")).toString().trimmed());
        if (settings.contains(QStringLiteral("rpcs3ActiveExeChoice")))
        {
            rpcs3ActiveExeChoice_ = settings.value(QStringLiteral("rpcs3ActiveExeChoice")).toInt();
        }
        else if (!rpcs3LatestExePath_.isEmpty())
        {
            rpcs3ActiveExeChoice_ = 1;
        }
        else if (!rpcs3CustomExePath_.isEmpty())
        {
            rpcs3ActiveExeChoice_ = 2;
        }
        rpcs3ActiveExeChoice_ = std::clamp(rpcs3ActiveExeChoice_, 0, 2);
        const int latestDefault = rpcs3LatestExePath_.isEmpty() ? rpcs3ActiveExeChoice_ : 1;
        npeb01406Rpcs3Choice_ = std::clamp(settings.value(QStringLiteral("npeb01406Rpcs3Choice"), 0).toInt(), 0, 2);
        npub31250Rpcs3Choice_ = std::clamp(settings.value(QStringLiteral("npub31250Rpcs3Choice"), latestDefault).toInt(), 0, 2);
        npjb00404Rpcs3Choice_ = std::clamp(settings.value(QStringLiteral("npjb00404Rpcs3Choice"), latestDefault).toInt(), 0, 2);
        npeb01406GamePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("npeb01406GamePath")).toString().trimmed());
        npub31250GamePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("npub31250GamePath")).toString().trimmed());
        npjb00404GamePath_ = QDir::toNativeSeparators(settings.value(QStringLiteral("npjb00404GamePath")).toString().trimmed());

        // Migrate older single-path config into the first EBOOT slot.
        const QString legacyGamePath = QDir::toNativeSeparators(settings.value(QStringLiteral("rpcs3GamePath")).toString().trimmed());
        if (npeb01406GamePath_.isEmpty() && npub31250GamePath_.isEmpty() && npjb00404GamePath_.isEmpty() && !legacyGamePath.isEmpty())
        {
            npeb01406GamePath_ = legacyGamePath;
        }

        refreshActiveGamePath();
        syncRpcs3SessionPaths();
        AppLogger::info(QStringLiteral("Migrating legacy RPCS3 configuration from '%1': default=%2; mappings=NPEB01406:%3, NPUB31250:%4, NPJB00404:%5; %6; %7; %8; %9; %10; %11")
                            .arg(QDir::toNativeSeparators(settings.fileName()),
                                 activeRpcs3Label(),
                     rpcs3LabelForChoice(npeb01406Rpcs3Choice_),
                     rpcs3LabelForChoice(npub31250Rpcs3Choice_),
                     rpcs3LabelForChoice(npjb00404Rpcs3Choice_),
                     pathDiagnostic(QStringLiteral("0.0.13"), rpcs3ExePath_),
                     pathDiagnostic(QStringLiteral("Latest"), rpcs3LatestExePath_),
                     pathDiagnostic(QStringLiteral("Custom"), rpcs3CustomExePath_),
                     pathDiagnostic(QStringLiteral("NPEB01406"), npeb01406GamePath_),
                     pathDiagnostic(QStringLiteral("NPUB31250"), npub31250GamePath_),
                     pathDiagnostic(QStringLiteral("NPJB00404"), npjb00404GamePath_)));
        saveRpcs3PathSetting();
    }

    void saveRpcs3PathSetting() const
    {
        const auto emulatorObject = [](const QString& path, const QString& build) {
            QJsonObject emulator;
            emulator.insert(QStringLiteral("path"), path.trimmed());
            emulator.insert(QStringLiteral("build"), build.trimmed());
            return emulator;
        };
        QJsonObject emulators;
        emulators.insert(QStringLiteral("0.0.13"), emulatorObject(rpcs3ExePath_, rpcs3Build_));
        emulators.insert(QStringLiteral("latest"), emulatorObject(rpcs3LatestExePath_, rpcs3LatestBuild_));
        emulators.insert(QStringLiteral("custom"), emulatorObject(rpcs3CustomExePath_, rpcs3CustomBuild_));

        const auto gameObject = [](const QString& bootPath, int emulatorChoice) {
            QJsonObject game;
            game.insert(QStringLiteral("bootPath"), bootPath.trimmed());
            game.insert(QStringLiteral("emulator"), emulatorChoice);
            return game;
        };
        QJsonObject games;
        games.insert(QStringLiteral("NPEB01406"), gameObject(npeb01406GamePath_, npeb01406Rpcs3Choice_));
        games.insert(QStringLiteral("NPUB31250"), gameObject(npub31250GamePath_, npub31250Rpcs3Choice_));
        games.insert(QStringLiteral("NPJB00404"), gameObject(npjb00404GamePath_, npjb00404Rpcs3Choice_));

        QJsonObject root;
        root.insert(QStringLiteral("version"), 3);
        root.insert(QStringLiteral("defaultEmulator"), rpcs3ActiveExeChoice_);
        root.insert(QStringLiteral("cheatEngine72Path"), cheatEngine72Path_.trimmed());
        root.insert(QStringLiteral("cheatEngine75Path"), cheatEngine75Path_.trimmed());
        root.insert(QStringLiteral("emulators"), emulators);
        root.insert(QStringLiteral("games"), games);

        const QString configurationPath = rpcs3ConfigurationPath();
        QDir().mkpath(QFileInfo(configurationPath).absolutePath());
        QSaveFile configurationFile(configurationPath);
        if (!configurationFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            AppLogger::error(QStringLiteral("Could not replace RPCS3 configuration file '%1': %2")
                                 .arg(QDir::toNativeSeparators(configurationPath), configurationFile.errorString()));
            return;
        }

        configurationFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        if (!configurationFile.commit())
        {
            AppLogger::error(QStringLiteral("Could not commit RPCS3 configuration file '%1': %2")
                                 .arg(QDir::toNativeSeparators(configurationPath), configurationFile.errorString()));
            return;
        }

        AppLogger::info(QStringLiteral("Replaced current RPCS3 configuration file: %1")
                            .arg(QDir::toNativeSeparators(configurationPath)));
    }

    void refreshActiveGamePath()
    {
        const std::array<QString, 3> candidates{
            npeb01406GamePath_.trimmed(),
            npub31250GamePath_.trimmed(),
            npjb00404GamePath_.trimmed()};

        rpcs3GamePath_.clear();
        for (const QString& candidate : candidates)
        {
            if (!candidate.isEmpty())
            {
                rpcs3GamePath_ = candidate;
                break;
            }
        }
    }

    void syncRpcs3SessionPaths()
    {
        const QString activeExePath = activeRpcs3ExePath();
        rpcs3Session_.setEmulatorPath(activeExePath);
        rpcs3Session_.setGamePath(rpcs3GamePath_);
        AppLogger::info(QStringLiteral("Synchronized RPCS3 session paths: active=%1; %2; %3")
                            .arg(activeRpcs3Label(),
                                 pathDiagnostic(QStringLiteral("exe"), activeExePath),
                                 pathDiagnostic(QStringLiteral("target"), rpcs3GamePath_)));
    }

    void showTransientInfoMessage(const QString& title, const QString& text, int closeDelayMs = 1400)
    {
        QMessageBox box(QMessageBox::Information, title, text, QMessageBox::Ok, this);
        box.setWindowModality(Qt::WindowModal);
        QTimer::singleShot(closeDelayMs, &box, [&box]() {
            box.accept();
        });
        box.exec();
    }

    void showRpcs3ConfigDialog()
    {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("RPCS3 Configuration"));

        auto* v = new QVBoxLayout(&dlg);
        auto* row = new QHBoxLayout();
        auto* label = new QLabel(QStringLiteral("RPCS3"), &dlg);
        auto* buildEdit = new QLineEdit(&dlg);
        buildEdit->setInputMask(QStringLiteral("0.0.00;_"));
        buildEdit->setPlaceholderText(QStringLiteral("0.0.00"));
        buildEdit->setText(rpcs3Build_);
        buildEdit->setFixedWidth(72);
        auto* pathEdit = new QLineEdit(&dlg);
        pathEdit->setText(rpcs3ExePath_);
        auto* browse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        row->addWidget(label);
        row->addWidget(buildEdit);
        row->addWidget(pathEdit, 1);
        row->addWidget(browse);
        v->addLayout(row);

        auto* latestRow = new QHBoxLayout();
        auto* latestLabel = new QLabel(QStringLiteral("RPCS3"), &dlg);
        auto* latestBuildEdit = new QLineEdit(&dlg);
        latestBuildEdit->setInputMask(QStringLiteral("0.0.00;_"));
        latestBuildEdit->setPlaceholderText(QStringLiteral("0.0.00"));
        latestBuildEdit->setText(rpcs3LatestBuild_);
        latestBuildEdit->setFixedWidth(72);
        auto* latestPathEdit = new QLineEdit(&dlg);
        latestPathEdit->setText(rpcs3LatestExePath_);
        auto* latestBrowse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        latestRow->addWidget(latestLabel);
        latestRow->addWidget(latestBuildEdit);
        latestRow->addWidget(latestPathEdit, 1);
        latestRow->addWidget(latestBrowse);
        v->addLayout(latestRow);

        auto* customRow = new QHBoxLayout();
        auto* customLabel = new QLabel(QStringLiteral("RPCS3"), &dlg);
        auto* customBuildEdit = new QLineEdit(&dlg);
        customBuildEdit->setInputMask(QStringLiteral("0.0.00;_"));
        customBuildEdit->setPlaceholderText(QStringLiteral("0.0.00"));
        customBuildEdit->setText(rpcs3CustomBuild_);
        customBuildEdit->setFixedWidth(72);
        auto* customPathEdit = new QLineEdit(&dlg);
        customPathEdit->setText(rpcs3CustomExePath_);
        auto* customBrowse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        customRow->addWidget(customLabel);
        customRow->addWidget(customBuildEdit);
        customRow->addWidget(customPathEdit, 1);
        customRow->addWidget(customBrowse);
        v->addLayout(customRow);

        auto* cheatEngine72Row = new QHBoxLayout();
        auto* cheatEngine72Label = new QLabel(QStringLiteral("CheatEngine 7.2:"), &dlg);
        auto* cheatEngine72PathEdit = new QLineEdit(&dlg);
        cheatEngine72PathEdit->setText(cheatEngine72Path_);
        auto* cheatEngine72Browse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        cheatEngine72Row->addWidget(cheatEngine72Label);
        cheatEngine72Row->addWidget(cheatEngine72PathEdit, 1);
        cheatEngine72Row->addWidget(cheatEngine72Browse);
        v->addLayout(cheatEngine72Row);

        auto* cheatEngine75Row = new QHBoxLayout();
        auto* cheatEngine75Label = new QLabel(QStringLiteral("CheatEngine 7.5:"), &dlg);
        auto* cheatEngine75PathEdit = new QLineEdit(&dlg);
        cheatEngine75PathEdit->setText(cheatEngine75Path_);
        auto* cheatEngine75Browse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        cheatEngine75Row->addWidget(cheatEngine75Label);
        cheatEngine75Row->addWidget(cheatEngine75PathEdit, 1);
        cheatEngine75Row->addWidget(cheatEngine75Browse);
        v->addLayout(cheatEngine75Row);

        auto* activeRow = new QHBoxLayout();
        auto* activeLabel = new QLabel(QStringLiteral("Default RPCS3:"), &dlg);
        auto* activeCombo = new QComboBox(&dlg);
        const QStringList emulatorLabels{
            QStringLiteral("RPCS3 %1").arg(rpcs3Build_),
            QStringLiteral("RPCS3 %1").arg(rpcs3LatestBuild_),
            QStringLiteral("RPCS3 %1").arg(rpcs3CustomBuild_)};
        activeCombo->addItems(emulatorLabels);
        activeCombo->setCurrentIndex(rpcs3ActiveExeChoice_);
        activeRow->addWidget(activeLabel);
        activeRow->addWidget(activeCombo, 1);
        v->addLayout(activeRow);

        auto* npebRow = new QHBoxLayout();
        auto* npebLabel = new QLabel(QStringLiteral("NPEB01406 EBOOT.BIN:"), &dlg);
        auto* npebEdit = new QLineEdit(&dlg);
        npebEdit->setText(npeb01406GamePath_);
        auto* npebBrowse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        auto* npebRpcs3Combo = new QComboBox(&dlg);
        npebRpcs3Combo->addItems(emulatorLabels);
        npebRpcs3Combo->setCurrentIndex(npeb01406Rpcs3Choice_);
        npebRow->addWidget(npebLabel);
        npebRow->addWidget(npebEdit, 1);
        npebRow->addWidget(npebRpcs3Combo);
        npebRow->addWidget(npebBrowse);
        v->addLayout(npebRow);

        auto* npubRow = new QHBoxLayout();
        auto* npubLabel = new QLabel(QStringLiteral("NPUB31250 EBOOT.BIN:"), &dlg);
        auto* npubEdit = new QLineEdit(&dlg);
        npubEdit->setText(npub31250GamePath_);
        auto* npubBrowse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        auto* npubRpcs3Combo = new QComboBox(&dlg);
        npubRpcs3Combo->addItems(emulatorLabels);
        npubRpcs3Combo->setCurrentIndex(npub31250Rpcs3Choice_);
        npubRow->addWidget(npubLabel);
        npubRow->addWidget(npubEdit, 1);
        npubRow->addWidget(npubRpcs3Combo);
        npubRow->addWidget(npubBrowse);
        v->addLayout(npubRow);

        auto* npjbRow = new QHBoxLayout();
        auto* npjbLabel = new QLabel(QStringLiteral("NPJB00404 EBOOT.BIN:"), &dlg);
        auto* npjbEdit = new QLineEdit(&dlg);
        npjbEdit->setText(npjb00404GamePath_);
        auto* npjbBrowse = new QPushButton(QStringLiteral("Browse..."), &dlg);
        auto* npjbRpcs3Combo = new QComboBox(&dlg);
        npjbRpcs3Combo->addItems(emulatorLabels);
        npjbRpcs3Combo->setCurrentIndex(npjb00404Rpcs3Choice_);
        npjbRow->addWidget(npjbLabel);
        npjbRow->addWidget(npjbEdit, 1);
        npjbRow->addWidget(npjbRpcs3Combo);
        npjbRow->addWidget(npjbBrowse);
        v->addLayout(npjbRow);

        auto* downloadRow = new QHBoxLayout();
        auto* download0013 = new QPushButton(QStringLiteral("Download RPCS3 0.0.13"), &dlg);
        auto* downloadLatest = new QPushButton(QStringLiteral("Download RPCS3 Latest"), &dlg);
        auto* downloadCheatEngine72 = new QPushButton(QStringLiteral("DownLoad CheatEngine 7.2"), &dlg);
        auto* downloadCheatEngine75 = new QPushButton(QStringLiteral("Download CheatEngine 7,5"), &dlg);
        downloadRow->addWidget(download0013);
        downloadRow->addWidget(downloadLatest);
        downloadRow->addWidget(downloadCheatEngine72);
        downloadRow->addWidget(downloadCheatEngine75);
        downloadRow->addStretch(1);
        v->addLayout(downloadRow);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        v->addWidget(buttons);

        connect(browse, &QPushButton::clicked, &dlg, [this, pathEdit]() {
            QString dir = QFileInfo(pathEdit->text().trimmed()).absolutePath();
            if (dir.isEmpty())
            {
                dir = QCoreApplication::applicationDirPath();
            }
            const QString selected = QFileDialog::getOpenFileName(this, QStringLiteral("Select RPCS3 Executable"), dir, QStringLiteral("Executables (*.exe)"));
            if (!selected.isEmpty())
            {
                pathEdit->setText(QDir::toNativeSeparators(selected));
            }
        });

        connect(latestBrowse, &QPushButton::clicked, &dlg, [this, latestPathEdit]() {
            QString dir = QFileInfo(latestPathEdit->text().trimmed()).absolutePath();
            if (dir.isEmpty())
            {
                dir = QCoreApplication::applicationDirPath();
            }
            const QString selected = QFileDialog::getOpenFileName(this, QStringLiteral("Select RPCS3 Latest Executable"), dir, QStringLiteral("Executables (*.exe)"));
            if (!selected.isEmpty())
            {
                latestPathEdit->setText(QDir::toNativeSeparators(selected));
            }
        });

        connect(customBrowse, &QPushButton::clicked, &dlg, [this, customPathEdit]() {
            QString dir = QFileInfo(customPathEdit->text().trimmed()).absolutePath();
            if (dir.isEmpty())
            {
                dir = QCoreApplication::applicationDirPath();
            }
            const QString selected = QFileDialog::getOpenFileName(this, QStringLiteral("Select RPCS3 Custom Executable"), dir, QStringLiteral("Executables (*.exe)"));
            if (!selected.isEmpty())
            {
                customPathEdit->setText(QDir::toNativeSeparators(selected));
            }
        });

        const auto connectCheatEngineBrowse = [this, &dlg](QPushButton* browseButton, QLineEdit* pathEdit, const QString& version) {
            connect(browseButton, &QPushButton::clicked, &dlg, [this, pathEdit, version]() {
                QString dir = QFileInfo(pathEdit->text().trimmed()).absolutePath();
                if (dir.isEmpty())
                {
                    dir = QCoreApplication::applicationDirPath();
                }
                const QString selected = QFileDialog::getOpenFileName(
                    this,
                    QStringLiteral("Select CheatEngine %1 Executable").arg(version),
                    dir,
                    QStringLiteral("Executables (*.exe)"));
                if (!selected.isEmpty())
                {
                    pathEdit->setText(QDir::toNativeSeparators(selected));
                }
            });
        };
        connectCheatEngineBrowse(cheatEngine72Browse, cheatEngine72PathEdit, QStringLiteral("7.2"));
        connectCheatEngineBrowse(cheatEngine75Browse, cheatEngine75PathEdit, QStringLiteral("7.5"));

        connect(download0013, &QPushButton::clicked, &dlg, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/RobertoTorino/rpcs3-trr")));
        });
        connect(downloadLatest, &QPushButton::clicked, &dlg, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/RPCS3/rpcs3/releases")));
        });
        connect(downloadCheatEngine72, &QPushButton::clicked, &dlg, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/RobertoTorino/cheat-engine-7.2-7.5-portable/releases")));
        });
        connect(downloadCheatEngine75, &QPushButton::clicked, &dlg, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/RobertoTorino/cheat-engine-7.2-7.5-portable/releases/tag/7.5-trr-main-4")));
        });

        const auto connectBrowse = [this, &dlg](QPushButton* browseButton, QLineEdit* targetEdit, const QString& title) {
            connect(browseButton, &QPushButton::clicked, &dlg, [this, targetEdit, title]() {
                QString dir = QFileInfo(targetEdit->text().trimmed()).absolutePath();
                if (dir.isEmpty())
                {
                    dir = QCoreApplication::applicationDirPath();
                }

                const QString selected = QFileDialog::getOpenFileName(
                    this,
                    title,
                    dir,
                    QStringLiteral("Boot targets (*.bin *.iso *.elf);;All files (*.*)"));
                if (!selected.isEmpty())
                {
                    targetEdit->setText(QDir::toNativeSeparators(selected));
                }
            });
        };

        connectBrowse(npebBrowse, npebEdit, QStringLiteral("Select NPEB01406 EBOOT.BIN"));
        connectBrowse(npubBrowse, npubEdit, QStringLiteral("Select NPUB31250 EBOOT.BIN"));
        connectBrowse(npjbBrowse, npjbEdit, QStringLiteral("Select NPJB00404 EBOOT.BIN"));

        connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        dlg.adjustSize();
        const QSize configSize = dlg.sizeHint();
        dlg.resize(configSize.width() * 2, configSize.height());

        if (dlg.exec() != QDialog::Accepted)
        {
            AppLogger::info(QStringLiteral("RPCS3 configuration dialog canceled; settings unchanged."));
            return;
        }

        const QString path = pathEdit->text().trimmed();
        const QString latestPath = latestPathEdit->text().trimmed();
        const QString customPath = customPathEdit->text().trimmed();
        const QString cheatEngine72Path = cheatEngine72PathEdit->text().trimmed();
        const QString cheatEngine75Path = cheatEngine75PathEdit->text().trimmed();
        const QString build = buildEdit->text().trimmed();
        const QString latestBuild = latestBuildEdit->text().trimmed();
        const QString customBuild = customBuildEdit->text().trimmed();
        const QRegularExpression buildPattern(QStringLiteral("^\\d\\.\\d\\.\\d{2}$"));
        if (!buildPattern.match(build).hasMatch() ||
            !buildPattern.match(latestBuild).hasMatch() ||
            !buildPattern.match(customBuild).hasMatch())
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: invalid build number format."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Every RPCS3 build must use the format 0.0.00."));
            return;
        }
        const int activeChoice = activeCombo->currentIndex();
        AppLogger::info(QStringLiteral("Validating RPCS3 configuration: active=%1; %2; %3; %4")
                            .arg(activeCombo->currentText(),
                                 pathDiagnostic(QStringLiteral("0.0.13"), path),
                                 pathDiagnostic(QStringLiteral("Latest"), latestPath),
                                 pathDiagnostic(QStringLiteral("Custom"), customPath)));
        if (path.isEmpty() && latestPath.isEmpty() && customPath.isEmpty())
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: all executable paths are empty."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Set at least one RPCS3 executable path."));
            return;
        }
        if ((!path.isEmpty() && !QFileInfo::exists(path)) ||
            (!latestPath.isEmpty() && !QFileInfo::exists(latestPath)) ||
            (!customPath.isEmpty() && !QFileInfo::exists(customPath)))
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: one or more executable paths do not exist."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("One or more RPCS3 executable paths do not exist."));
            return;
        }
        if ((!cheatEngine72Path.isEmpty() && !QFileInfo(cheatEngine72Path).isFile()) ||
            (!cheatEngine75Path.isEmpty() && !QFileInfo(cheatEngine75Path).isFile()))
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: a CheatEngine executable does not exist."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("One or more CheatEngine executable paths do not exist."));
            return;
        }

        const std::array<QString, 3> executablePaths{path, latestPath, customPath};
        if (activeChoice < 0 || activeChoice >= static_cast<int>(executablePaths.size()) || executablePaths[activeChoice].isEmpty())
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: selected game-launch executable is empty."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("The RPCS3 selected for game launch has no executable path."));
            return;
        }

        const QString npebPath = npebEdit->text().trimmed();
        const QString npubPath = npubEdit->text().trimmed();
        const QString npjbPath = npjbEdit->text().trimmed();
        const std::array<int, 3> gameChoices{
            npebRpcs3Combo->currentIndex(),
            npubRpcs3Combo->currentIndex(),
            npjbRpcs3Combo->currentIndex()};
        const std::array<QString, 3> gamePaths{npebPath, npubPath, npjbPath};
        const std::array<QString, 3> titleIds{
            QStringLiteral("NPEB01406"),
            QStringLiteral("NPUB31250"),
            QStringLiteral("NPJB00404")};
        for (std::size_t i = 0; i < gameChoices.size(); ++i)
        {
            if (!gamePaths[i].isEmpty() && executablePaths[static_cast<std::size_t>(gameChoices[i])].isEmpty())
            {
                AppLogger::error(QStringLiteral("RPCS3 configuration rejected: %1 maps to an empty executable slot.").arg(titleIds[i]));
                QMessageBox::warning(this,
                                     QStringLiteral("TRR Qt Trainer"),
                                     QStringLiteral("%1 is assigned to an RPCS3 executable path that is empty.").arg(titleIds[i]));
                return;
            }
        }
        if ((!npebPath.isEmpty() && !QFileInfo::exists(npebPath)) ||
            (!npubPath.isEmpty() && !QFileInfo::exists(npubPath)) ||
            (!npjbPath.isEmpty() && !QFileInfo::exists(npjbPath)))
        {
            AppLogger::error(QStringLiteral("RPCS3 configuration rejected: one or more EBOOT paths do not exist. %1; %2; %3")
                                 .arg(pathDiagnostic(QStringLiteral("NPEB01406"), npebPath),
                                      pathDiagnostic(QStringLiteral("NPUB31250"), npubPath),
                                      pathDiagnostic(QStringLiteral("NPJB00404"), npjbPath)));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("One or more EBOOT paths are set but do not exist."));
            return;
        }

        rpcs3ExePath_ = QDir::toNativeSeparators(path);
        rpcs3LatestExePath_ = QDir::toNativeSeparators(latestPath);
        rpcs3CustomExePath_ = QDir::toNativeSeparators(customPath);
        cheatEngine72Path_ = QDir::toNativeSeparators(cheatEngine72Path);
        cheatEngine75Path_ = QDir::toNativeSeparators(cheatEngine75Path);
        rpcs3Build_ = build;
        rpcs3LatestBuild_ = latestBuild;
        rpcs3CustomBuild_ = customBuild;
        rpcs3ActiveExeChoice_ = activeChoice;
        npeb01406Rpcs3Choice_ = gameChoices[0];
        npub31250Rpcs3Choice_ = gameChoices[1];
        npjb00404Rpcs3Choice_ = gameChoices[2];
        npeb01406GamePath_ = QDir::toNativeSeparators(npebPath);
        npub31250GamePath_ = QDir::toNativeSeparators(npubPath);
        npjb00404GamePath_ = QDir::toNativeSeparators(npjbPath);
        refreshActiveGamePath();
        syncRpcs3SessionPaths();
        saveRpcs3PathSetting();
        AppLogger::info(QStringLiteral("RPCS3 configuration saved: default=%1; mappings=NPEB01406:%2, NPUB31250:%3, NPJB00404:%4; %5")
                    .arg(activeRpcs3Label(),
                     rpcs3LabelForChoice(npeb01406Rpcs3Choice_),
                     rpcs3LabelForChoice(npub31250Rpcs3Choice_),
                     rpcs3LabelForChoice(npjb00404Rpcs3Choice_),
                     pathDiagnostic(QStringLiteral("defaultExe"), activeRpcs3ExePath())));
        statusLabel_->setText(QStringLiteral("Status: RPCS3 config saved"));
    }

    void showStartRpcs3Dialog()
    {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("Start RPCS3"));
        auto* layout = new QVBoxLayout(&dlg);
        auto* start0013 = new QPushButton(QStringLiteral("Start RPCS3 0.0.13"), &dlg);
        auto* startLatest = new QPushButton(QStringLiteral("Start RPCS3 Latest"), &dlg);
        auto* startCustom = new QPushButton(QStringLiteral("Start RPCS3 Custom"), &dlg);
        auto* cancel = new QDialogButtonBox(QDialogButtonBox::Cancel, &dlg);
        layout->addWidget(start0013);
        layout->addWidget(startLatest);
        layout->addWidget(startCustom);
        layout->addWidget(cancel);

        int choice = -1;
        connect(start0013, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 0;
            dlg.accept();
        });
        connect(startLatest, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 1;
            dlg.accept();
        });
        connect(startCustom, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 2;
            dlg.accept();
        });
        connect(cancel, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() != QDialog::Accepted)
        {
            return;
        }

        if (choice == 0)
        {
            startRpcs3(rpcs3ExePath_, QStringLiteral("RPCS3 %1").arg(rpcs3Build_), 0);
        }
        else if (choice == 1)
        {
            startRpcs3(rpcs3LatestExePath_, QStringLiteral("RPCS3 %1").arg(rpcs3LatestBuild_), 1);
        }
        else if (choice == 2)
        {
            startRpcs3(rpcs3CustomExePath_, QStringLiteral("RPCS3 %1").arg(rpcs3CustomBuild_), 2);
        }
    }

    void showStartGameDialog()
    {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("Start Game"));
        auto* layout = new QVBoxLayout(&dlg);
        auto* startNpeb = new QPushButton(QStringLiteral("Start NPEB01406"), &dlg);
        auto* startNpub = new QPushButton(QStringLiteral("Start NPUB31250"), &dlg);
        auto* startNpjb = new QPushButton(QStringLiteral("Start NPJB00404"), &dlg);
        auto* cancel = new QDialogButtonBox(QDialogButtonBox::Cancel, &dlg);
        layout->addWidget(startNpeb);
        layout->addWidget(startNpub);
        layout->addWidget(startNpjb);
        layout->addWidget(cancel);

        int choice = -1;
        connect(startNpeb, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 0;
            dlg.accept();
        });
        connect(startNpub, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 1;
            dlg.accept();
        });
        connect(startNpjb, &QPushButton::clicked, &dlg, [&dlg, &choice]() {
            choice = 2;
            dlg.accept();
        });
        connect(cancel, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() != QDialog::Accepted)
        {
            return;
        }

        if (choice == 0)
        {
            startConfiguredGame(npeb01406GamePath_, QStringLiteral("NPEB01406"));
        }
        else if (choice == 1)
        {
            startConfiguredGame(npub31250GamePath_, QStringLiteral("NPUB31250"));
        }
        else if (choice == 2)
        {
            startConfiguredGame(npjb00404GamePath_, QStringLiteral("NPJB00404"));
        }
    }

    bool startRpcs3(const QString& emulatorPath, const QString& versionName, int emulatorChoice)
    {
        rpcs3Session_.setEmulatorPath(emulatorPath);
        rpcs3Session_.setGamePath(rpcs3GamePath_);
        AppLogger::info(QStringLiteral("Start %1 requested. exe=%2").arg(versionName, emulatorPath));
        QString error;
        if (!rpcs3Session_.startEmulator(&error))
        {
            AppLogger::error(QStringLiteral("Start %1 failed: %2").arg(versionName, error));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), error);
            return false;
        }

        AppLogger::info(QStringLiteral("Start %1 launch requested successfully.").arg(versionName));
        runningGameLabel_->setText(QStringLiteral("Game: Unknown"));
        runningBuildLabel_->setText(QStringLiteral("Build: %1").arg(rpcs3BuildForChoice(emulatorChoice)));
        statusLabel_->setText(QStringLiteral("Status: %1 launch requested").arg(versionName));
        return true;
    }

    bool resetEmulator()
    {
        syncRpcs3SessionPaths();
        AppLogger::info(QStringLiteral("Reset RPCS3 requested. active=%1 exe=%2")
                    .arg(activeRpcs3Label(), activeRpcs3ExePath()));
        stopRuntimeWrites(false);
        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }

        QString error;
        if (!rpcs3Session_.resetEmulator(500, &error))
        {
            AppLogger::error(QStringLiteral("Reset RPCS3 failed: %1").arg(error));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), error);
            return false;
        }

        battlePtr_ = 0;
        pointerLabel_->setText(QStringLiteral("Battle Pointer: Unresolved"));
        attachButton_->setChecked(false);
        attachButton_->setText(QStringLiteral("Attach RPCS3"));
        updateLiveMonitor();

        AppLogger::info(QStringLiteral("Reset RPCS3 launch requested successfully."));
        runningGameLabel_->setText(QStringLiteral("Game: Unknown"));
        runningBuildLabel_->setText(QStringLiteral("Build: %1").arg(activeRpcs3Label()));
        statusLabel_->setText(QStringLiteral("Status: RPCS3 reset requested"));
        return true;
    }

    bool startConfiguredGame(const QString& gamePath, const QString& titleId)
    {
        const int emulatorChoice = rpcs3ChoiceForTitle(titleId);
        const QString emulatorPath = rpcs3ExePathForChoice(emulatorChoice);
        rpcs3GamePath_ = QDir::toNativeSeparators(gamePath.trimmed());
        lastLaunchedTitleId_ = titleId;
        rpcs3Session_.setEmulatorPath(emulatorPath);
        rpcs3Session_.setGamePath(rpcs3GamePath_);
        AppLogger::info(QStringLiteral("Start %1 requested. active=%2 exe=%3 target=%4")
                            .arg(titleId, rpcs3LabelForChoice(emulatorChoice), emulatorPath, rpcs3GamePath_));
        QString error;
        if (!rpcs3Session_.startGame(&error))
        {
            AppLogger::error(QStringLiteral("Start %1 failed: %2").arg(titleId, error));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), error);
            return false;
        }

        AppLogger::info(QStringLiteral("Start %1 launch requested successfully.").arg(titleId));
        runningGameLabel_->setText(QStringLiteral("Game: %1").arg(titleId));
        runningBuildLabel_->setText(QStringLiteral("Build: %1").arg(rpcs3BuildForChoice(emulatorChoice)));
        statusLabel_->setText(QStringLiteral("Status: %1 launch requested").arg(titleId));

        QTimer::singleShot(1500, this, [this]() {
            if (process_ == nullptr)
            {
                attach();
            }
            else
            {
                refreshPointer();
            }
        });

        return true;
    }

    bool terminateRpcs3()
    {
        syncRpcs3SessionPaths();
        AppLogger::info(QStringLiteral("Terminate RPCS3 requested."));
        stopRuntimeWrites(false);
        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }

        QString error;
        if (!rpcs3Session_.stopEmulatorSession(&error))
        {
            AppLogger::error(QStringLiteral("Terminate RPCS3 failed: %1").arg(error));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), error);
            return false;
        }

        battlePtr_ = 0;
        pointerLabel_->setText(QStringLiteral("Battle Pointer: Unresolved"));
        attachButton_->setChecked(false);
        attachButton_->setText(QStringLiteral("Attach RPCS3"));
        updateLiveMonitor();

        AppLogger::info(QStringLiteral("Terminate RPCS3 completed."));
        runningGameLabel_->setText(QStringLiteral("Game: Unknown"));
        runningBuildLabel_->setText(QStringLiteral("Build: Unknown"));
        statusLabel_->setText(QStringLiteral("Status: RPCS3 terminated"));
        return true;
    }

    bool restartConfiguredGame()
    {
        const int emulatorChoice = lastLaunchedTitleId_.isEmpty()
                                       ? rpcs3ActiveExeChoice_
                                       : rpcs3ChoiceForTitle(lastLaunchedTitleId_);
        const QString emulatorPath = rpcs3ExePathForChoice(emulatorChoice);
        rpcs3Session_.setEmulatorPath(emulatorPath);
        rpcs3Session_.setGamePath(rpcs3GamePath_);
        AppLogger::info(QStringLiteral("Restart Game requested. active=%1 exe=%2 target=%3")
                            .arg(rpcs3LabelForChoice(emulatorChoice), emulatorPath, rpcs3GamePath_));
        stopRuntimeWrites(false);
        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }

        QString error;
        if (!rpcs3Session_.restartGame(500, &error))
        {
            AppLogger::error(QStringLiteral("Restart Game failed: %1").arg(error));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), error);
            return false;
        }

        AppLogger::info(QStringLiteral("Restart Game launch requested successfully."));
        runningGameLabel_->setText(lastLaunchedTitleId_.isEmpty()
                                       ? QStringLiteral("Game: Unknown")
                                       : QStringLiteral("Game: %1").arg(lastLaunchedTitleId_));
        runningBuildLabel_->setText(QStringLiteral("Build: %1").arg(rpcs3BuildForChoice(emulatorChoice)));
        statusLabel_->setText(QStringLiteral("Status: game restart requested"));
        stageLockArmed_ = true;

        QTimer::singleShot(1500, this, [this]() {
            attach();
            refreshPointer();
        });

        return true;
    }

    void openBuildTutorial()
    {
        const QString trainerRoot = QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("..")));
        const QString readmePath = QDir(trainerRoot).filePath(QStringLiteral("README.md"));
        const QString automationPath = QDir(trainerRoot).filePath(QStringLiteral("README_AUTOMATION.md"));

        const auto loadMarkdown = [](const QString& path, const QString& title) {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                return QStringLiteral("# %1\n\nCould not open:\n\n`%2`")
                    .arg(title, QDir::toNativeSeparators(path));
            }

            const QByteArray bytes = file.readAll();
            if (bytes.isEmpty())
            {
                return QStringLiteral("# %1\n\nFile is empty:\n\n`%2`")
                    .arg(title, QDir::toNativeSeparators(path));
            }

            return QString::fromUtf8(bytes);
        };

        if (tutorialDialog_ == nullptr)
        {
            tutorialDialog_ = new QDialog(this);
            tutorialDialog_->setWindowTitle(QStringLiteral("Build | Tutorial"));

            auto* layout = new QVBoxLayout(tutorialDialog_);

            auto* split = new QSplitter(Qt::Horizontal, tutorialDialog_);
            auto* navPanel = new QWidget(split);
            auto* navLayout = new QVBoxLayout(navPanel);
            navLayout->setContentsMargins(6, 6, 6, 6);
            navLayout->setSpacing(6);

            auto* searchLabel = new QLabel(QStringLiteral("Search"), navPanel);
            tutorialSearchEdit_ = new QLineEdit(navPanel);
            tutorialSearchEdit_->setPlaceholderText(QStringLiteral("Find text in current tab"));
            auto* searchButtonsRow = new QHBoxLayout();
            auto* searchPrevButton = new QPushButton(QStringLiteral("Previous"), navPanel);
            auto* searchNextButton = new QPushButton(QStringLiteral("Next"), navPanel);
            searchButtonsRow->addWidget(searchPrevButton);
            searchButtonsRow->addWidget(searchNextButton);

            auto* sectionsLabel = new QLabel(QStringLiteral("Sections"), navPanel);
            tutorialSectionFilterEdit_ = new QLineEdit(navPanel);
            tutorialSectionFilterEdit_->setPlaceholderText(QStringLiteral("Filter sections"));
            tutorialSectionList_ = new QListWidget(navPanel);

            navLayout->addWidget(searchLabel);
            navLayout->addWidget(tutorialSearchEdit_);
            navLayout->addLayout(searchButtonsRow);
            navLayout->addSpacing(8);
            navLayout->addWidget(sectionsLabel);
            navLayout->addWidget(tutorialSectionFilterEdit_);
            navLayout->addWidget(tutorialSectionList_, 1);

            tutorialTabs_ = new QTabWidget(split);
            tutorialReadmeView_ = new QTextBrowser(tutorialDialog_);
            tutorialAutomationView_ = new QTextBrowser(tutorialDialog_);

            tutorialReadmeView_->setOpenExternalLinks(true);
            tutorialAutomationView_->setOpenExternalLinks(true);

            tutorialTabs_->addTab(tutorialReadmeView_, QStringLiteral("Qt Trainer README"));
            tutorialTabs_->addTab(tutorialAutomationView_, QStringLiteral("CE Automation README"));

            split->addWidget(navPanel);
            split->addWidget(tutorialTabs_);
            split->setStretchFactor(0, 0);
            split->setStretchFactor(1, 1);
            split->setSizes({280, 720});

            layout->addWidget(split);

            auto* actions = new QDialogButtonBox(QDialogButtonBox::Close, tutorialDialog_);
            auto* openPdfButton = new QPushButton(QStringLiteral("Open README.pdf"), tutorialDialog_);
            actions->addButton(openPdfButton, QDialogButtonBox::ActionRole);
            layout->addWidget(actions);

            connect(actions, &QDialogButtonBox::rejected, tutorialDialog_, &QDialog::reject);
            connect(tutorialTabs_, &QTabWidget::currentChanged, this, [this](int) {
                rebuildTutorialSectionIndex();
            });
            connect(tutorialSearchEdit_, &QLineEdit::returnPressed, this, [this]() { searchTutorial(true); });
            connect(searchPrevButton, &QPushButton::clicked, this, [this]() { searchTutorial(false); });
            connect(searchNextButton, &QPushButton::clicked, this, [this]() { searchTutorial(true); });
            connect(tutorialSectionFilterEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
                rebuildTutorialSectionIndex();
            });
            connect(tutorialSectionList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
                jumpToTutorialSection(item);
            });
            connect(tutorialSectionList_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
                jumpToTutorialSection(item);
            });

            auto* shortcutFocusSearch = new QShortcut(QKeySequence::Find, tutorialDialog_);
            connect(shortcutFocusSearch, &QShortcut::activated, this, [this]() {
                if (tutorialSearchEdit_ != nullptr)
                {
                    tutorialSearchEdit_->setFocus(Qt::ShortcutFocusReason);
                    tutorialSearchEdit_->selectAll();
                }
            });

            auto* shortcutSearchNext = new QShortcut(QKeySequence(Qt::Key_F3), tutorialDialog_);
            connect(shortcutSearchNext, &QShortcut::activated, this, [this]() {
                searchTutorial(true);
            });

            auto* shortcutSearchPrevious = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3), tutorialDialog_);
            connect(shortcutSearchPrevious, &QShortcut::activated, this, [this]() {
                searchTutorial(false);
            });

            connect(openPdfButton, &QPushButton::clicked, this, [this]() {
                const QString pdfPath = QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../README.pdf")));
                AppLogger::info(QStringLiteral("Build | Tutorial PDF requested: %1").arg(pdfPath));

                if (!QFileInfo::exists(pdfPath))
                {
                    QMessageBox::warning(this,
                                         QStringLiteral("TRR Qt Trainer"),
                                         QStringLiteral("README.pdf was not found at:\n%1").arg(QDir::toNativeSeparators(pdfPath)));
                    return;
                }

                if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath)))
                {
                    QMessageBox::warning(this,
                                         QStringLiteral("TRR Qt Trainer"),
                                         QStringLiteral("Failed to open README.pdf."));
                }
            });

            tutorialDialog_->resize(1024, 720);
        }

        AppLogger::info(QStringLiteral("Build | Tutorial dialog opened. root=%1").arg(QDir::toNativeSeparators(trainerRoot)));
        tutorialReadmeMarkdown_ = loadMarkdown(readmePath, QStringLiteral("Qt Trainer README"));
        tutorialAutomationMarkdown_ = loadMarkdown(automationPath, QStringLiteral("CE Automation README"));

        // Resolve relative markdown assets (e.g., assets/images/TRR_256.png) from trainer root.
        const QStringList tutorialSearchPaths{trainerRoot};
        tutorialReadmeView_->setSearchPaths(tutorialSearchPaths);
        tutorialAutomationView_->setSearchPaths(tutorialSearchPaths);

        tutorialReadmeView_->setMarkdown(tutorialReadmeMarkdown_);
        tutorialAutomationView_->setMarkdown(tutorialAutomationMarkdown_);
        rebuildTutorialSectionIndex();

        tutorialDialog_->show();
        tutorialDialog_->raise();
        tutorialDialog_->activateWindow();
    }

    QTextBrowser* currentTutorialView() const
    {
        if (tutorialTabs_ == nullptr)
        {
            return nullptr;
        }

        return tutorialTabs_->currentIndex() == 1 ? tutorialAutomationView_ : tutorialReadmeView_;
    }

    void searchTutorial(bool forward)
    {
        QTextBrowser* view = currentTutorialView();
        if (view == nullptr || tutorialSearchEdit_ == nullptr)
        {
            return;
        }

        const QString term = tutorialSearchEdit_->text().trimmed();
        if (term.isEmpty())
        {
            return;
        }

        QTextDocument::FindFlags flags;
        if (!forward)
        {
            flags |= QTextDocument::FindBackward;
        }

        QTextDocument* doc = view->document();
        QTextCursor found = doc->find(term, view->textCursor(), flags);

        if (found.isNull())
        {
            QTextCursor wrapCursor(doc);
            wrapCursor.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
            found = doc->find(term, wrapCursor, flags);
        }

        if (found.isNull())
        {
            showTransientInfoMessage(QStringLiteral("Build | Tutorial"), QStringLiteral("No matches for '%1'.").arg(term), 1200);
            return;
        }

        view->setTextCursor(found);
        view->ensureCursorVisible();
    }

    void rebuildTutorialSectionIndex()
    {
        if (tutorialSectionList_ == nullptr || tutorialTabs_ == nullptr)
        {
            return;
        }

        tutorialSectionList_->clear();

        const QString markdown = tutorialTabs_->currentIndex() == 1 ? tutorialAutomationMarkdown_ : tutorialReadmeMarkdown_;
        const QString filter = tutorialSectionFilterEdit_ != nullptr ? tutorialSectionFilterEdit_->text().trimmed() : QString();
        const QStringList lines = markdown.split('\n');

        for (const QString& line : lines)
        {
            const QString trimmed = line.trimmed();
            if (!trimmed.startsWith(QLatin1Char('#')))
            {
                continue;
            }

            int level = 0;
            while (level < trimmed.size() && trimmed.at(level) == QLatin1Char('#'))
            {
                ++level;
            }

            if (level <= 0 || level >= trimmed.size())
            {
                continue;
            }

            const QString heading = trimmed.mid(level).trimmed();
            if (heading.isEmpty())
            {
                continue;
            }

            if (!filter.isEmpty() && !heading.contains(filter, Qt::CaseInsensitive))
            {
                continue;
            }

            const QString display = QString(level > 1 ? (level - 1) * 2 : 0, QLatin1Char(' ')) + heading;
            auto* item = new QListWidgetItem(display, tutorialSectionList_);
            item->setData(Qt::UserRole, heading);
        }
    }

    void jumpToTutorialSection(QListWidgetItem* item)
    {
        if (item == nullptr)
        {
            return;
        }

        QTextBrowser* view = currentTutorialView();
        if (view == nullptr)
        {
            return;
        }

        const QString heading = item->data(Qt::UserRole).toString();
        if (heading.isEmpty())
        {
            return;
        }

        QTextCursor start(view->document());
        start.movePosition(QTextCursor::Start);

        QTextCursor found = view->document()->find(heading, start, QTextDocument::FindCaseSensitively);
        if (found.isNull())
        {
            found = view->document()->find(heading, start);
        }

        if (!found.isNull())
        {
            view->setTextCursor(found);
            view->ensureCursorVisible();
        }
    }

    QString snapshotGameBucket() const
    {
        const QString path = QDir::fromNativeSeparators(rpcs3GamePath_);
        QRegularExpression rx(QStringLiteral("/game/([^/]+)/"), QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = rx.match(path);
        if (match.hasMatch())
        {
            return match.captured(1).toUpper();
        }
        return QStringLiteral("Generic");
    }

    bool takeSnapshot()
    {
        HWND targetHwnd = nullptr;

        const auto pid = rpcs3Session_.findRunningPid();
        if (pid.has_value())
        {
            targetHwnd = findTopLevelWindowByPid(pid.value());
        }

        if (targetHwnd == nullptr)
        {
            targetHwnd = GetForegroundWindow();
        }

        if (targetHwnd == nullptr)
        {
            AppLogger::warn(QStringLiteral("Snapshot failed: no target window found."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("No window found to capture."));
            return false;
        }

        QScreen* screen = QApplication::primaryScreen();
        if (screen == nullptr)
        {
            AppLogger::error(QStringLiteral("Snapshot failed: primary screen is unavailable."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("No screen available for capture."));
            return false;
        }

        const QPixmap pixmap = screen->grabWindow(reinterpret_cast<WId>(targetHwnd));
        if (pixmap.isNull())
        {
            AppLogger::error(QStringLiteral("Snapshot failed: captured pixmap is empty."));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Capture failed. The target window may be minimized or protected."));
            return false;
        }

        const QString snapshotsDir = QDir(QCoreApplication::applicationDirPath())
                                         .filePath(QStringLiteral("media/snapshots/%1").arg(snapshotGameBucket()));
        QDir().mkpath(snapshotsDir);

        const QString baseExe = QFileInfo(rpcs3ExePath_).completeBaseName().trimmed();
        const QString exeName = baseExe.isEmpty() ? QStringLiteral("Window") : baseExe;
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss_zzz"));
        QString filePath = QDir(snapshotsDir).filePath(QStringLiteral("%1_%2.png").arg(exeName, stamp));

        int suffix = 1;
        while (QFileInfo::exists(filePath))
        {
            filePath = QDir(snapshotsDir).filePath(QStringLiteral("%1_%2_%3.png").arg(exeName, stamp).arg(suffix));
            ++suffix;
        }

        if (!pixmap.save(filePath, "PNG"))
        {
            AppLogger::error(QStringLiteral("Snapshot save failed: %1").arg(filePath));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Failed to save snapshot."));
            return false;
        }

        AppLogger::info(QStringLiteral("Snapshot saved: %1").arg(filePath));
        statusLabel_->setText(QStringLiteral("Status: Snapshot saved"));
        return true;
    }

    bool attach()
    {
        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }

        syncRpcs3SessionPaths();
        auto pid = rpcs3Session_.findRunningPid();

        if (!pid.has_value())
        {
            AppLogger::warn(QStringLiteral("Attach failed: RPCS3 process not found."));
            runningGameLabel_->setText(QStringLiteral("Game: Unknown"));
            runningBuildLabel_->setText(QStringLiteral("Build: Unknown"));
            statusLabel_->setText(QStringLiteral("Status: RPCS3 process not found"));
            attachButton_->setChecked(false);
            attachButton_->setText(QStringLiteral("Attach RPCS3"));
            return false;
        }

        process_ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid.value());
        if (process_ == nullptr)
        {
            AppLogger::warn(QStringLiteral("Attach failed: OpenProcess failed for pid %1").arg(pid.value()));
            statusLabel_->setText(QStringLiteral("Status: OpenProcess failed (run trainer as admin?)"));
            attachButton_->setChecked(false);
            attachButton_->setText(QStringLiteral("Attach RPCS3"));
            return false;
        }

        AppLogger::info(QStringLiteral("Attached to RPCS3 pid=%1").arg(pid.value()));
        statusLabel_->setText(QStringLiteral("Status: Attached to PID %1").arg(pid.value()));
        attachButton_->setChecked(true);
        attachButton_->setText(QStringLiteral("Detach RPCS3"));
        refreshPointer();
        return true;
    }

    void detach()
    {
        AppLogger::info(QStringLiteral("Detach requested."));
        stopRuntimeWrites(false);

        if (process_ != nullptr)
        {
            CloseHandle(process_);
            process_ = nullptr;
        }

        battlePtr_ = 0;
        pointerLabel_->setText(QStringLiteral("Battle Pointer: Unresolved"));
        runningGameLabel_->setText(QStringLiteral("Game: Unknown"));
        runningBuildLabel_->setText(QStringLiteral("Build: Unknown"));
        statusLabel_->setText(QStringLiteral("Status: Detached"));
        attachButton_->setChecked(false);
        attachButton_->setText(QStringLiteral("Attach RPCS3"));

        updateLiveMonitor();
    }

    void showLogs()
    {
        const QString logPath = AppLogger::logFilePath();
        if (!QFileInfo::exists(logPath))
        {
            QFile seed(logPath);
            if (seed.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                seed.close();
            }
        }

        AppLogger::info(QStringLiteral("Show Logs requested: %1").arg(logPath));
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(logPath)))
        {
            QProcess::startDetached(QStringLiteral("notepad.exe"), {logPath});
        }
    }

    void showStartCheatEngineDialog()
    {
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("Start CheatEngine"));
        auto* layout = new QVBoxLayout(&dlg);
        auto* start72Button = new QPushButton(QStringLiteral("Start CE 7.2"), &dlg);
        auto* start75Button = new QPushButton(QStringLiteral("Start CE 7.5"), &dlg);
        layout->addWidget(start72Button);
        layout->addWidget(start75Button);

        connect(start72Button, &QPushButton::clicked, &dlg, [this, &dlg]() {
            dlg.accept();
            startCheatEngine(cheatEngine72Path_, QStringLiteral("7.2"));
        });
        connect(start75Button, &QPushButton::clicked, &dlg, [this, &dlg]() {
            dlg.accept();
            startCheatEngine(cheatEngine75Path_, QStringLiteral("7.5"));
        });

        dlg.exec();
    }

    void startCheatEngine(const QString& configuredPath, const QString& version)
    {
        const QString path = configuredPath.trimmed();
        if (path.isEmpty() || !QFileInfo(path).isFile())
        {
            QMessageBox::warning(this,
                                 QStringLiteral("TRR Qt Trainer"),
                                 QStringLiteral("Configure a valid CheatEngine %1 executable in RPCS3 Config first.").arg(version));
            return;
        }

        AppLogger::info(QStringLiteral("Start CheatEngine %1 requested: %2").arg(version, QDir::toNativeSeparators(path)));
        if (!QProcess::startDetached(path, {}))
        {
            AppLogger::error(QStringLiteral("Failed to start CheatEngine %1: %2").arg(version, QDir::toNativeSeparators(path)));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Failed to start CheatEngine %1.").arg(version));
            return;
        }

        statusLabel_->setText(QStringLiteral("Status: CheatEngine %1 launch requested").arg(version));
    }

    void openTrManual()
    {
        const QString manualPath = QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../assets/manual/TekkenRevolutionManual.pdf")));
        AppLogger::info(QStringLiteral("TR Manual requested: %1").arg(manualPath));

        if (!QFileInfo::exists(manualPath))
        {
            QMessageBox::warning(this,
                                 QStringLiteral("TRR Qt Trainer"),
                                 QStringLiteral("TR Manual PDF was not found at:\n%1").arg(QDir::toNativeSeparators(manualPath)));
            return;
        }

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(manualPath)))
        {
            QMessageBox::warning(this,
                                 QStringLiteral("TRR Qt Trainer"),
                                 QStringLiteral("Failed to open TR Manual PDF."));
        }
    }

    void openE32013()
    {
        const QUrl url(QStringLiteral("https://www.youtube.com/watch?v=oCqIseOnkqA"));
        AppLogger::info(QStringLiteral("E3 2013 requested: %1").arg(url.toString()));

        const QString edgePath = QStringLiteral("C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe");
        if (QFileInfo::exists(edgePath))
        {
            if (QProcess::startDetached(edgePath, {QStringLiteral("--start-fullscreen"), url.toString()}))
            {
                return;
            }

            AppLogger::warn(QStringLiteral("Edge fullscreen launch failed, falling back to default browser."));
        }

        if (!QDesktopServices::openUrl(url))
        {
            QMessageBox::warning(this,
                                 QStringLiteral("TRR Qt Trainer"),
                                 QStringLiteral("Failed to open the E3 2013 video link."));
        }
    }

    bool refreshPointer()
    {
        if (process_ == nullptr && !attach())
        {
            return false;
        }

        std::optional<std::uint64_t> ptr;
        for (int i = 0; i < kPointerRetryCount; ++i)
        {
            ptr = resolveBattlePtr(process_);
            if (ptr.has_value())
            {
                break;
            }
            Sleep(kPointerRetryDelayMs);
        }

        if (!ptr.has_value())
        {
            battlePtr_ = 0;
            pointerLabel_->setText(QStringLiteral("Battle Pointer: Unresolved (go through splash/demo and retry)"));
            return false;
        }

        battlePtr_ = ptr.value();
        pointerLabel_->setText(QStringLiteral("Battle Pointer: %1").arg(formatHex64(battlePtr_)));
        return true;
    }

    std::optional<std::uint32_t> readBattleU32(std::uint32_t offset) const
    {
        if (process_ == nullptr || battlePtr_ == 0)
        {
            return std::nullopt;
        }
        return readBE32(process_, battlePtr_ + offset);
    }

    bool shouldDisableStageLock() const
    {
        const auto timerValue = readBattleU32(kOffRoundTimer);
        return timerValue.has_value() && timerValue.value() > 0;
    }

    void updateLiveMonitor()
    {
        if (process_ == nullptr || battlePtr_ == 0)
        {
            monitorP1Id_->setText(QStringLiteral("P1 id: n/a"));
            monitorP2Id_->setText(QStringLiteral("P2 id: n/a"));
            monitorStage_->setText(QStringLiteral("Stage id: n/a"));
            monitorState_->setText(QStringLiteral("Game State: n/a"));
            monitorTimer_->setText(QStringLiteral("Round Timer: n/a"));
            monitorCounters_->setText(QStringLiteral("Counters P1/P2: n/a"));
            monitorUi_->setText(QStringLiteral("UI Flags: n/a"));
            monitorInf_->setText(QStringLiteral("Infinite Round: n/a"));
            return;
        }

        const auto p1 = readBattleU32(kOffP1Id);
        const auto p2 = readBattleU32(kOffP2Id);
        const auto stage = readBattleU32(kOffStageId);
        const auto state = readBattleU32(kOffGameState);
        const auto timer = readBattleU32(kOffRoundTimer);
        const auto c1 = readBattleU32(kOffCounterP1);
        const auto c2 = readBattleU32(kOffCounterP2);
        const auto ui = readBattleU32(kOffUiFlags);
        const auto inf = readBattleU32(kOffInfiniteRound);

        monitorP1Id_->setText(p1.has_value() ? QStringLiteral("P1 id: %1").arg(formatHex32(p1.value())) : QStringLiteral("P1 ID: n/a"));
        monitorP2Id_->setText(p2.has_value() ? QStringLiteral("P2 id: %1").arg(formatHex32(p2.value())) : QStringLiteral("P2 ID: n/a"));
        monitorStage_->setText(stage.has_value() ? QStringLiteral("Stage id: %1").arg(formatHex32(stage.value())) : QStringLiteral("Stage ID: n/a"));
        monitorState_->setText(state.has_value() ? QStringLiteral("Game State: %1").arg(formatHex32(state.value())) : QStringLiteral("Game state: n/a"));
        monitorTimer_->setText(timer.has_value() ? QStringLiteral("Round Timer: %1").arg(timer.value()) : QStringLiteral("Round timer: n/a"));
        monitorCounters_->setText((c1.has_value() && c2.has_value()) ? QStringLiteral("Counters P1/P2: %1 / %2").arg(c1.value()).arg(c2.value()) : QStringLiteral("Counters P1/P2: n/a"));
        monitorUi_->setText(ui.has_value() ? QStringLiteral("UI Flags: %1").arg(formatHex32(ui.value())) : QStringLiteral("UI flags: n/a"));
        monitorInf_->setText(inf.has_value() ? QStringLiteral("Infinite Round: %1").arg(formatHex32(inf.value())) : QStringLiteral("Infinite round: n/a"));
    }

    QJsonObject buildPresetObject() const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("version"), 3);
        obj.insert(QStringLiteral("p1Index"), p1Combo_->currentIndex());
        obj.insert(QStringLiteral("p2Index"), p2Combo_->currentIndex());
        obj.insert(QStringLiteral("stageIndex"), stageCombo_->currentIndex());
        obj.insert(QStringLiteral("lockValues"), lockCheckbox_->isChecked());
        obj.insert(QStringLiteral("lockSelection"), lockSelectionCheckbox_->isChecked());
        obj.insert(QStringLiteral("autoDisableStageLock"), autoDisableStageLockCheckbox_->isChecked());
        obj.insert(QStringLiteral("transitionGuard"), guardPauseCheckbox_->isChecked());
        obj.insert(QStringLiteral("transitionGuardMs"), guardPauseMsSpin_->value());
        obj.insert(QStringLiteral("stabilize"), stabilizeCheckbox_->isChecked());
        obj.insert(QStringLiteral("modeResetPulse"), modeResetPulseCheckbox_->isChecked());
        obj.insert(QStringLiteral("writeMode"), writeModeCheckbox_->isChecked());
        obj.insert(QStringLiteral("modePresetIndex"), modePresetCombo_->currentIndex());
        obj.insert(QStringLiteral("writeHp"), writeHpCheckbox_->isChecked());
        obj.insert(QStringLiteral("hpPresetSelected"), hpPresetCheckbox_->isChecked());
        obj.insert(QStringLiteral("hpRandomSelected"), hpRandomCheckbox_->isChecked());
        obj.insert(QStringLiteral("hpPresetIndex"), hpPresetCombo_->currentIndex());
        obj.insert(QStringLiteral("hpValue"), hpEdit_->text());
        obj.insert(QStringLiteral("writeInfiniteRound"), writeInfiniteRoundCheckbox_->isChecked());
        obj.insert(QStringLiteral("infiniteRoundValue"), infiniteRoundSpin_->value());
        obj.insert(QStringLiteral("p1Controller"), p1ControllerCheckbox_->isChecked());
        obj.insert(QStringLiteral("p2Cpu"), p2CpuCheckbox_->isChecked());
        obj.insert(QStringLiteral("forceTimer"), roundTimerCheckbox_->isChecked());
        obj.insert(QStringLiteral("forceCounters"), countersCheckbox_->isChecked());
        obj.insert(QStringLiteral("timerSeconds"), roundTimerSecondsSpin_->value());
        obj.insert(QStringLiteral("roundTimePresetIndex"), roundTimePresetCombo_->currentIndex());
        obj.insert(QStringLiteral("p1Counter"), p1CounterSpin_->value());
        obj.insert(QStringLiteral("p2Counter"), p2CounterSpin_->value());
        return obj;
    }

    void applyPresetObject(const QJsonObject& obj)
    {
        const auto setIndexSafe = [](QComboBox* combo, int idx) {
            if (idx >= 0 && idx < combo->count())
            {
                combo->setCurrentIndex(idx);
            }
        };

        setIndexSafe(p1Combo_, obj.value(QStringLiteral("p1Index")).toInt(p1Combo_->currentIndex()));
        setIndexSafe(p2Combo_, obj.value(QStringLiteral("p2Index")).toInt(p2Combo_->currentIndex()));
        setIndexSafe(stageCombo_, obj.value(QStringLiteral("stageIndex")).toInt(stageCombo_->currentIndex()));

        lockCheckbox_->setChecked(obj.value(QStringLiteral("lockValues")).toBool(lockCheckbox_->isChecked()));
        lockSelectionCheckbox_->setChecked(obj.value(QStringLiteral("lockSelection")).toBool(lockSelectionCheckbox_->isChecked()));
        autoDisableStageLockCheckbox_->setChecked(obj.value(QStringLiteral("autoDisableStageLock")).toBool(autoDisableStageLockCheckbox_->isChecked()));
        guardPauseCheckbox_->setChecked(obj.value(QStringLiteral("transitionGuard")).toBool(guardPauseCheckbox_->isChecked()));
        guardPauseMsSpin_->setValue(obj.value(QStringLiteral("transitionGuardMs")).toInt(guardPauseMsSpin_->value()));
        stabilizeCheckbox_->setChecked(obj.value(QStringLiteral("stabilize")).toBool(stabilizeCheckbox_->isChecked()));
        modeResetPulseCheckbox_->setChecked(obj.value(QStringLiteral("modeResetPulse")).toBool(modeResetPulseCheckbox_->isChecked()));
        writeModeCheckbox_->setChecked(obj.value(QStringLiteral("writeMode")).toBool(writeModeCheckbox_->isChecked()));
        writeHpCheckbox_->setChecked(obj.value(QStringLiteral("writeHp")).toBool(writeHpCheckbox_->isChecked()));
        hpPresetCheckbox_->setChecked(obj.value(QStringLiteral("hpPresetSelected")).toBool(hpPresetCheckbox_->isChecked()));
        hpRandomCheckbox_->setChecked(obj.value(QStringLiteral("hpRandomSelected")).toBool(hpRandomCheckbox_->isChecked()));
        setIndexSafe(hpPresetCombo_, obj.value(QStringLiteral("hpPresetIndex")).toInt(hpPresetCombo_->currentIndex()));
        writeInfiniteRoundCheckbox_->setChecked(obj.value(QStringLiteral("writeInfiniteRound")).toBool(writeInfiniteRoundCheckbox_->isChecked()));
        hpEdit_->setText(obj.value(QStringLiteral("hpValue")).toString(hpEdit_->text()));
        infiniteRoundSpin_->setValue(obj.value(QStringLiteral("infiniteRoundValue")).toInt(infiniteRoundSpin_->value()));
        p1ControllerCheckbox_->setChecked(obj.value(QStringLiteral("p1Controller")).toBool(p1ControllerCheckbox_->isChecked()));
        p2CpuCheckbox_->setChecked(obj.value(QStringLiteral("p2Cpu")).toBool(p2CpuCheckbox_->isChecked()));
        roundTimerCheckbox_->setChecked(obj.value(QStringLiteral("forceTimer")).toBool(roundTimerCheckbox_->isChecked()));
        countersCheckbox_->setChecked(obj.value(QStringLiteral("forceCounters")).toBool(countersCheckbox_->isChecked()));
        roundTimerSecondsSpin_->setValue(obj.value(QStringLiteral("timerSeconds")).toInt(roundTimerSecondsSpin_->value()));
        setIndexSafe(modePresetCombo_, obj.value(QStringLiteral("modePresetIndex")).toInt(modePresetCombo_->currentIndex()));
        if (obj.value(QStringLiteral("modePresetIndex")).isUndefined() && obj.contains(QStringLiteral("modeValue")))
        {
            const int legacyMode = obj.value(QStringLiteral("modeValue")).toInt(-1);
            const int legacyIndex = modePresetCombo_->findData(static_cast<quint32>(legacyMode));
            if (legacyIndex >= 0)
            {
                modePresetCombo_->setCurrentIndex(legacyIndex);
            }
        }
        setIndexSafe(roundTimePresetCombo_, obj.value(QStringLiteral("roundTimePresetIndex")).toInt(roundTimePresetCombo_->currentIndex()));
        p1CounterSpin_->setValue(obj.value(QStringLiteral("p1Counter")).toInt(p1CounterSpin_->value()));
        p2CounterSpin_->setValue(obj.value(QStringLiteral("p2Counter")).toInt(p2CounterSpin_->value()));
        stageLockArmed_ = true;
    }

    void applyStabilityProfile(int profile)
    {
        switch (profile)
        {
        case 0:
            lockCheckbox_->setChecked(true);
            lockSelectionCheckbox_->setChecked(false);
            autoDisableStageLockCheckbox_->setChecked(true);
            guardPauseCheckbox_->setChecked(true);
            guardPauseMsSpin_->setValue(3600);
            stabilizeCheckbox_->setChecked(true);
            modeResetPulseCheckbox_->setChecked(false);
            writeModeCheckbox_->setChecked(true);
            modePresetCombo_->setCurrentIndex(modePresetCombo_->findData(static_cast<quint32>(5)));
            writeHpCheckbox_->setChecked(true);
            hpPresetCheckbox_->setChecked(false);
            hpRandomCheckbox_->setChecked(true);
            hpEdit_->setText(QStringLiteral("0x10000000"));
            writeInfiniteRoundCheckbox_->setChecked(true);
            infiniteRoundSpin_->setValue(1);
            p1ControllerCheckbox_->setChecked(true);
            p2CpuCheckbox_->setChecked(true);
            roundTimerCheckbox_->setChecked(true);
            roundTimerSecondsSpin_->setValue(60);
            countersCheckbox_->setChecked(false);
            statusLabel_->setText(QStringLiteral("Status: applied Conservative profile"));
            break;
        case 1:
            lockCheckbox_->setChecked(true);
            lockSelectionCheckbox_->setChecked(false);
            autoDisableStageLockCheckbox_->setChecked(true);
            guardPauseCheckbox_->setChecked(true);
            guardPauseMsSpin_->setValue(2400);
            stabilizeCheckbox_->setChecked(true);
            modeResetPulseCheckbox_->setChecked(false);
            writeModeCheckbox_->setChecked(false);
            writeHpCheckbox_->setChecked(false);
            writeInfiniteRoundCheckbox_->setChecked(false);
            p1ControllerCheckbox_->setChecked(true);
            p2CpuCheckbox_->setChecked(true);
            roundTimerCheckbox_->setChecked(false);
            countersCheckbox_->setChecked(false);
            statusLabel_->setText(QStringLiteral("Status: Applied Balanced Profile"));
            break;
        case 2:
            lockCheckbox_->setChecked(true);
            lockSelectionCheckbox_->setChecked(true);
            autoDisableStageLockCheckbox_->setChecked(false);
            guardPauseCheckbox_->setChecked(false);
            guardPauseMsSpin_->setValue(1200);
            stabilizeCheckbox_->setChecked(true);
            modeResetPulseCheckbox_->setChecked(true);
            writeModeCheckbox_->setChecked(true);
            modePresetCombo_->setCurrentIndex(modePresetCombo_->findData(static_cast<quint32>(1)));
            writeHpCheckbox_->setChecked(true);
            hpPresetCheckbox_->setChecked(false);
            hpRandomCheckbox_->setChecked(true);
            hpEdit_->setText(QStringLiteral("0x10000000"));
            writeInfiniteRoundCheckbox_->setChecked(true);
            infiniteRoundSpin_->setValue(1);
            p1ControllerCheckbox_->setChecked(true);
            p2CpuCheckbox_->setChecked(true);
            roundTimerCheckbox_->setChecked(true);
            roundTimerSecondsSpin_->setValue(90);
            countersCheckbox_->setChecked(true);
            statusLabel_->setText(QStringLiteral("Status: Applied Aggressive Profile"));
            break;
        default:
            return;
        }

        stageLockArmed_ = true;
    }

    int guardWindowTicks() const
    {
        const int interval = timer_ != nullptr ? timer_->interval() : 200;
        int ticks = guardPauseMsSpin_->value() / interval;
        if ((guardPauseMsSpin_->value() % interval) != 0)
        {
            ++ticks;
        }
        return ticks < 1 ? 1 : ticks;
    }

    void updateRoundTransitionGuard()
    {
        if (!guardPauseCheckbox_->isChecked())
        {
            guardTicksRemaining_ = 0;
        }

        if (process_ == nullptr || battlePtr_ == 0)
        {
            prevTimer_.reset();
            prevCounterP1_.reset();
            prevCounterP2_.reset();
            return;
        }

        const auto timerValue = readBattleU32(kOffRoundTimer);
        const auto c1 = readBattleU32(kOffCounterP1);
        const auto c2 = readBattleU32(kOffCounterP2);

        bool transitionDetected = false;
        if (timerValue.has_value() && prevTimer_.has_value())
        {
            if (prevTimer_.value() > 0 && timerValue.value() == 0)
            {
                transitionDetected = true;
            }
            if (timerValue.value() > prevTimer_.value() + 300)
            {
                transitionDetected = true;
            }
        }
        if (c1.has_value() && prevCounterP1_.has_value() && c1.value() != prevCounterP1_.value())
        {
            transitionDetected = true;
        }
        if (c2.has_value() && prevCounterP2_.has_value() && c2.value() != prevCounterP2_.value())
        {
            transitionDetected = true;
        }

        if (guardPauseCheckbox_->isChecked() && transitionDetected)
        {
            guardTicksRemaining_ = guardWindowTicks();
        }

        prevTimer_ = timerValue;
        prevCounterP1_ = c1;
        prevCounterP2_ = c2;
    }

    QString defaultPresetDirectory() const
    {
        const QDir appDir(QCoreApplication::applicationDirPath());
        const QString presetDirectory = QDir::cleanPath(appDir.filePath(QStringLiteral("presets")));
        QDir().mkpath(presetDirectory);
        return presetDirectory;
    }

    void savePreset()
    {
        const QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Save Preset"), defaultPresetDirectory() + QStringLiteral("/trr_preset.json"), QStringLiteral("JSON files (*.json)"));
        if (filePath.isEmpty())
        {
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could Not Write Preset File."));
            return;
        }

        file.write(QJsonDocument(buildPresetObject()).toJson(QJsonDocument::Indented));
        file.close();
        statusLabel_->setText(QStringLiteral("Status: Preset Saved"));
    }

    void loadPreset()
    {
        const QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Load Preset"), defaultPresetDirectory(), QStringLiteral("JSON files (*.json)"));
        if (filePath.isEmpty())
        {
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could Not Read Preset File."));
            return;
        }

        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject())
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Invalid Preset JSON."));
            return;
        }

        applyPresetObject(doc.object());
        statusLabel_->setText(QStringLiteral("Status: Preset Loaded"));
    }

    bool writeAndVerifyU32(std::uint64_t addr, std::uint32_t expected)
    {
        if (process_ == nullptr)
        {
            return false;
        }

        for (int i = 0; i < kVerifyRetryCount; ++i)
        {
            if (writeBE32(process_, addr, expected))
            {
                const auto actual = readBE32(process_, addr);
                if (actual.has_value() && actual.value() == expected)
                {
                    return true;
                }
            }
            Sleep(kVerifyRetryDelayMs);
        }

        return false;
    }

    bool parseWriteValues(std::uint32_t& modeValue, std::uint32_t& hpValue, std::uint32_t& infiniteValue, std::uint32_t& timerTicks, bool showError)
    {
        modeValue = modePresetCombo_->currentData().toUInt();
        infiniteValue = static_cast<std::uint32_t>(infiniteRoundSpin_->value());

        if (writeHpCheckbox_->isChecked())
        {
            if (hpPresetCheckbox_->isChecked())
            {
                hpValue = hpPresetCombo_->currentData().toUInt();
            }
            else
            {
                const auto parsed = parseU32Input(hpEdit_->text());
                if (!parsed.has_value())
                {
                    if (showError)
                    {
                        QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Invalid HP/UI value. Use decimal or 0x hex."));
                    }
                    return false;
                }
                hpValue = parsed.value();
            }
        }
        else
        {
            hpValue = 0;
        }

        const std::uint64_t timerTicks64 = static_cast<std::uint64_t>(roundTimerSecondsSpin_->value()) * kRoundTimerTicksPerSecond;
        if (timerTicks64 > 0xFFFFFFFFULL)
        {
            if (showError)
            {
                QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Round timer seconds are too large."));
            }
            return false;
        }

        timerTicks = static_cast<std::uint32_t>(timerTicks64);
        return true;
    }

    bool writeValuesFast(bool writeCharacters, bool writeStage, bool writeState)
    {
        if (process_ == nullptr || battlePtr_ == 0)
        {
            return false;
        }

        std::uint32_t modeValue = 0;
        std::uint32_t hpValue = 0;
        std::uint32_t infiniteValue = 0;
        std::uint32_t timerTicks = 0;
        if (!parseWriteValues(modeValue, hpValue, infiniteValue, timerTicks, false))
        {
            return false;
        }

        bool ok = true;
        if (writeCharacters)
        {
            ok = writeBE32(process_, battlePtr_ + kOffP1Id, p1Combo_->currentData().toUInt()) && ok;
            ok = writeBE32(process_, battlePtr_ + kOffP2Id, p2Combo_->currentData().toUInt()) && ok;
        }

        if (writeStage)
        {
            ok = writeBE32(process_, battlePtr_ + kOffStageId, stageCombo_->currentData().toUInt()) && ok;
        }

        if (writeState && writeModeCheckbox_->isChecked())
        {
            if (modeResetPulseCheckbox_->isChecked() && modeValue != 4)
            {
                ok = writeBE32(process_, battlePtr_ + kOffGameState, 4) && ok;
                Sleep(kModeResetPulseMs);
            }
            ok = writeBE32(process_, battlePtr_ + kOffGameState, modeValue) && ok;
        }

        if (writeState && writeHpCheckbox_->isChecked())
        {
            ok = writeBE32(process_, battlePtr_ + kOffUiFlags, hpValue) && ok;
        }

        if (writeState && writeInfiniteRoundCheckbox_->isChecked())
        {
            ok = writeBE32(process_, battlePtr_ + kOffInfiniteRound, infiniteValue) && ok;
        }

        if (writeState && roundTimerCheckbox_->isChecked())
        {
            ok = writeBE32(process_, battlePtr_ + kOffRoundTimer, timerTicks) && ok;
        }

        if (writeState && countersCheckbox_->isChecked())
        {
            ok = writeBE32(process_, battlePtr_ + kOffCounterP1, static_cast<std::uint32_t>(p1CounterSpin_->value())) && ok;
            ok = writeBE32(process_, battlePtr_ + kOffCounterP2, static_cast<std::uint32_t>(p2CounterSpin_->value())) && ok;
        }

        if (writeState && p1ControllerCheckbox_->isChecked())
        {
            ok = writeBE32(process_, kAddrP1State, 0) && ok;
        }

        if (writeState && p2CpuCheckbox_->isChecked())
        {
            ok = writeBE32(process_, kAddrP2State, 1) && ok;
        }

        return ok;
    }

    void stopRuntimeWrites(bool updateStatus)
    {
        runtimeLockEnabled_ = false;
        runtimeStabilizeCyclesLeft_ = 0;
        if (updateStatus)
        {
            statusLabel_->setText(QStringLiteral("Status: continuous lock stopped"));
        }
    }

    bool readLiveValuesIntoUi()
    {
        if (process_ == nullptr && !attach())
        {
            statusLabel_->setText(QStringLiteral("Status: Failed to Attach"));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could not attach to RPCS3 process."));
            return false;
        }

        if (!refreshPointer())
        {
            statusLabel_->setText(QStringLiteral("Status: Failed to Resolve Pointer"));
            return false;
        }

        const auto mode = readBattleU32(kOffGameState);
        const auto hp = readBattleU32(kOffUiFlags);
        const auto inf = readBattleU32(kOffInfiniteRound);
        const auto timerTicks = readBattleU32(kOffRoundTimer);

        if (mode.has_value())
        {
            const int modeIndex = modePresetCombo_->findData(static_cast<quint32>(mode.value()));
            if (modeIndex >= 0)
            {
                modePresetCombo_->setCurrentIndex(modeIndex);
            }
        }
        if (hp.has_value())
        {
            const int presetIndex = hpPresetCombo_->findData(static_cast<quint32>(hp.value()));
            if (presetIndex >= 0)
            {
                hpPresetCombo_->setCurrentIndex(presetIndex);
            }
            hpEdit_->setText(formatHex32(hp.value()));
        }
        if (inf.has_value())
        {
            infiniteRoundSpin_->setValue(static_cast<int>(inf.value()));
        }
        if (timerTicks.has_value())
        {
            const auto seconds = static_cast<int>((timerTicks.value() + (kRoundTimerTicksPerSecond / 2U)) / kRoundTimerTicksPerSecond);
            roundTimerSecondsSpin_->setValue(seconds);
        }

        statusLabel_->setText(QStringLiteral("Status: live values read"));
        return true;
    }

    void showRuntimeDialog()
    {
        runtimeDialog_->setFixedSize(runtimeDialog_->minimumSizeHint());
        runtimeDialog_->show();
        runtimeDialog_->raise();
        runtimeDialog_->activateWindow();
    }

    void showValueWritesDialog()
    {
        valueWritesDialog_->setFixedSize(valueWritesDialog_->minimumSizeHint());
        valueWritesDialog_->show();
        valueWritesDialog_->raise();
        valueWritesDialog_->activateWindow();
    }

    void showAdvancedMemoryDialog()
    {
        if (advancedMemoryDialog_ == nullptr)
        {
            advancedMemoryDialog_ = new QDialog(this);
            advancedMemoryDialog_->setWindowTitle(QStringLiteral("Advanced Memory"));

            auto* dialogLayout = new QVBoxLayout(advancedMemoryDialog_);
            auto* advancedBox = new QGroupBox(QStringLiteral("Advanced Memory"), advancedMemoryDialog_);
            auto* advancedGrid = new QGridLayout(advancedBox);
            advancedP1PositionCheckbox_ = new QCheckBox(QStringLiteral("Write P1 position"), advancedBox);
            advancedP2PositionCheckbox_ = new QCheckBox(QStringLiteral("Write P2 position"), advancedBox);
            advancedAnimationCheckbox_ = new QCheckBox(QStringLiteral("Write P1 animation speed"), advancedBox);
            advancedGlobalStageCheckbox_ = new QCheckBox(QStringLiteral("Write global stage ID"), advancedBox);

            const auto configurePositionSpin = [](QDoubleSpinBox* spin) {
                spin->setRange(-1000000.0, 1000000.0);
                spin->setDecimals(4);
                spin->setFixedWidth(110);
            };
            advancedP1XSpin_ = new QDoubleSpinBox(advancedBox);
            advancedP1YSpin_ = new QDoubleSpinBox(advancedBox);
            advancedP1ZSpin_ = new QDoubleSpinBox(advancedBox);
            advancedP2XSpin_ = new QDoubleSpinBox(advancedBox);
            advancedP2YSpin_ = new QDoubleSpinBox(advancedBox);
            advancedP2ZSpin_ = new QDoubleSpinBox(advancedBox);
            for (QDoubleSpinBox* spin : {advancedP1XSpin_, advancedP1YSpin_, advancedP1ZSpin_, advancedP2XSpin_, advancedP2YSpin_, advancedP2ZSpin_})
            {
                configurePositionSpin(spin);
            }

            advancedAnimationEdit_ = new QLineEdit(QStringLiteral("0x00001000"), advancedBox);
            advancedGlobalStageEdit_ = new QLineEdit(QStringLiteral("0x00000000"), advancedBox);
            advancedAnimationEdit_->setFixedWidth(110);
            advancedGlobalStageEdit_->setFixedWidth(110);
            advancedGameStateLabel_ = new QLabel(QStringLiteral("Game state: n/a"), advancedBox);
            advancedGameStateReadLabel_ = new QLabel(QStringLiteral("Game state (read): n/a"), advancedBox);
            advancedReadButton_ = new QPushButton(QStringLiteral("Read Advanced Values"), advancedBox);
            advancedApplyButton_ = new QPushButton(QStringLiteral("Apply Checked Values"), advancedBox);

            advancedGrid->addWidget(advancedP1PositionCheckbox_, 0, 0);
            advancedGrid->addWidget(new QLabel(QStringLiteral("X:"), advancedBox), 0, 1);
            advancedGrid->addWidget(advancedP1XSpin_, 0, 2);
            advancedGrid->addWidget(new QLabel(QStringLiteral("Y:"), advancedBox), 0, 3);
            advancedGrid->addWidget(advancedP1YSpin_, 0, 4);
            advancedGrid->addWidget(new QLabel(QStringLiteral("Z:"), advancedBox), 0, 5);
            advancedGrid->addWidget(advancedP1ZSpin_, 0, 6);
            advancedGrid->addWidget(advancedP2PositionCheckbox_, 1, 0);
            advancedGrid->addWidget(new QLabel(QStringLiteral("X:"), advancedBox), 1, 1);
            advancedGrid->addWidget(advancedP2XSpin_, 1, 2);
            advancedGrid->addWidget(new QLabel(QStringLiteral("Y:"), advancedBox), 1, 3);
            advancedGrid->addWidget(advancedP2YSpin_, 1, 4);
            advancedGrid->addWidget(new QLabel(QStringLiteral("Z:"), advancedBox), 1, 5);
            advancedGrid->addWidget(advancedP2ZSpin_, 1, 6);
            advancedGrid->addWidget(advancedAnimationCheckbox_, 2, 0, 1, 2);
            advancedGrid->addWidget(advancedAnimationEdit_, 2, 2);
            advancedGrid->addWidget(advancedGlobalStageCheckbox_, 3, 0, 1, 2);
            advancedGrid->addWidget(advancedGlobalStageEdit_, 3, 2);
            advancedGrid->addWidget(advancedGameStateLabel_, 4, 0, 1, 3);
            advancedGrid->addWidget(advancedGameStateReadLabel_, 4, 3, 1, 4);

            auto* actionRowWidget = new QWidget(advancedBox);
            auto* actionRow = new QHBoxLayout(actionRowWidget);
            actionRow->setContentsMargins(0, 0, 0, 0);
            actionRow->addWidget(advancedReadButton_);
            actionRow->addWidget(advancedApplyButton_);
            actionRow->addStretch(1);
            advancedGrid->addWidget(actionRowWidget, 5, 0, 1, 7);
            dialogLayout->addWidget(advancedBox);

            auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, advancedMemoryDialog_);
            dialogLayout->addWidget(buttons);
            connect(advancedReadButton_, &QPushButton::clicked, this, [this]() { readAdvancedValuesIntoUi(); });
            connect(advancedApplyButton_, &QPushButton::clicked, this, [this]() { applyAdvancedValues(); });
            connect(buttons, &QDialogButtonBox::rejected, advancedMemoryDialog_, &QDialog::reject);

            advancedMemoryDialog_->setFixedSize(advancedMemoryDialog_->minimumSizeHint());
        }

        advancedMemoryDialog_->show();
        advancedMemoryDialog_->raise();
        advancedMemoryDialog_->activateWindow();
    }

    bool readAdvancedValuesIntoUi()
    {
        if (process_ == nullptr && !attach())
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could not attach to RPCS3 process."));
            return false;
        }

        const auto p1X = readBEFloat(process_, kAddrP1PosX);
        const auto p1Y = readBEFloat(process_, kAddrP1PosY);
        const auto p1Z = readBEFloat(process_, kAddrP1PosZ);
        const auto p2X = readBEFloat(process_, kAddrP2PosX);
        const auto p2Y = readBEFloat(process_, kAddrP2PosY);
        const auto p2Z = readBEFloat(process_, kAddrP2PosZ);
        const auto animationSpeed = readBE32(process_, kAddrP1AnimationSpeed);
        const auto gameState = readBE32(process_, kAddrGameState);
        const auto gameStateRead = readBE32(process_, kAddrGameStateRead);
        const auto globalStage = readBE32(process_, kAddrGlobalStageId);

        const bool positionsValid = p1X.has_value() && p1Y.has_value() && p1Z.has_value() &&
                                    p2X.has_value() && p2Y.has_value() && p2Z.has_value() &&
                                    std::isfinite(p1X.value()) && std::isfinite(p1Y.value()) && std::isfinite(p1Z.value()) &&
                                    std::isfinite(p2X.value()) && std::isfinite(p2Y.value()) && std::isfinite(p2Z.value());
        if (!positionsValid)
        {
            statusLabel_->setText(QStringLiteral("Status: failed to read advanced positions"));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Advanced position values could not be read for RPCS3 01.05."));
            return false;
        }

        advancedP1XSpin_->setValue(p1X.value());
        advancedP1YSpin_->setValue(p1Y.value());
        advancedP1ZSpin_->setValue(p1Z.value());
        advancedP2XSpin_->setValue(p2X.value());
        advancedP2YSpin_->setValue(p2Y.value());
        advancedP2ZSpin_->setValue(p2Z.value());
        advancedAnimationEdit_->setText(animationSpeed.has_value() ? formatHex32(animationSpeed.value()) : QStringLiteral("n/a"));
        advancedGlobalStageEdit_->setText(globalStage.has_value() ? formatHex32(globalStage.value()) : QStringLiteral("n/a"));
        advancedGameStateLabel_->setText(gameState.has_value() ? QStringLiteral("Game state: %1").arg(formatHex32(gameState.value())) : QStringLiteral("Game state: n/a"));
        advancedGameStateReadLabel_->setText(gameStateRead.has_value() ? QStringLiteral("Game state (read): %1").arg(formatHex32(gameStateRead.value())) : QStringLiteral("Game state (read): n/a"));
        statusLabel_->setText(QStringLiteral("Status: advanced values read"));
        return true;
    }

    bool applyAdvancedValues()
    {
        if (process_ == nullptr && !attach())
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could not attach to RPCS3 process."));
            return false;
        }

        if (!advancedP1PositionCheckbox_->isChecked() && !advancedP2PositionCheckbox_->isChecked() &&
            !advancedAnimationCheckbox_->isChecked() && !advancedGlobalStageCheckbox_->isChecked())
        {
            QMessageBox::information(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Select at least one advanced write option."));
            return false;
        }

        bool allOk = true;
        if (advancedP1PositionCheckbox_->isChecked())
        {
            allOk = writeAndVerifyU32(kAddrP1PosX, floatBits(static_cast<float>(advancedP1XSpin_->value()))) && allOk;
            allOk = writeAndVerifyU32(kAddrP1PosY, floatBits(static_cast<float>(advancedP1YSpin_->value()))) && allOk;
            allOk = writeAndVerifyU32(kAddrP1PosZ, floatBits(static_cast<float>(advancedP1ZSpin_->value()))) && allOk;
        }
        if (advancedP2PositionCheckbox_->isChecked())
        {
            allOk = writeAndVerifyU32(kAddrP2PosX, floatBits(static_cast<float>(advancedP2XSpin_->value()))) && allOk;
            allOk = writeAndVerifyU32(kAddrP2PosY, floatBits(static_cast<float>(advancedP2YSpin_->value()))) && allOk;
            allOk = writeAndVerifyU32(kAddrP2PosZ, floatBits(static_cast<float>(advancedP2ZSpin_->value()))) && allOk;
        }
        if (advancedAnimationCheckbox_->isChecked())
        {
            const auto value = parseU32Input(advancedAnimationEdit_->text());
            if (!value.has_value())
            {
                QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Invalid P1 animation speed. Use decimal or 0x hex."));
                return false;
            }
            allOk = writeAndVerifyU32(kAddrP1AnimationSpeed, value.value()) && allOk;
        }
        if (advancedGlobalStageCheckbox_->isChecked())
        {
            const auto value = parseU32Input(advancedGlobalStageEdit_->text());
            if (!value.has_value())
            {
                QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Invalid global stage ID. Use decimal or 0x hex."));
                return false;
            }
            allOk = writeAndVerifyU32(kAddrGlobalStageId, value.value()) && allOk;
        }

        statusLabel_->setText(allOk ? QStringLiteral("Status: advanced values applied") : QStringLiteral("Status: advanced write failed"));
        if (!allOk)
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("One or more advanced values failed verification."));
        }
        return allOk;
    }

    bool applySelection()
    {
        if (process_ == nullptr && !attach())
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Could not attach to RPCS3 process."));
            return false;
        }

        if (!refreshPointer())
        {
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("Battle pointer unresolved. Enter splash/demo path first."));
            return false;
        }

        std::uint32_t modeValue = 0;
        std::uint32_t hpValue = 0;
        std::uint32_t infiniteValue = 0;
        std::uint32_t timerTicks = 0;
        if (!parseWriteValues(modeValue, hpValue, infiniteValue, timerTicks, true))
        {
            return false;
        }

        bool allOk = true;
        QString checks;
        const auto appendCheck = [&checks](const QString& line) {
            if (!checks.isEmpty())
            {
                checks.append('\n');
            }
            checks.append(line);
        };

        const bool p1Ok = writeAndVerifyU32(battlePtr_ + kOffP1Id, p1Combo_->currentData().toUInt());
        const bool p2Ok = writeAndVerifyU32(battlePtr_ + kOffP2Id, p2Combo_->currentData().toUInt());
        const bool stageOk = writeAndVerifyU32(battlePtr_ + kOffStageId, stageCombo_->currentData().toUInt());
        allOk = allOk && p1Ok && p2Ok && stageOk;
        appendCheck(QStringLiteral("P1 ID: %1").arg(p1Ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        appendCheck(QStringLiteral("P2 ID: %1").arg(p2Ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        appendCheck(QStringLiteral("Stage ID: %1").arg(stageOk ? QStringLiteral("OK") : QStringLiteral("FAIL")));

        if (writeModeCheckbox_->isChecked())
        {
            if (modeResetPulseCheckbox_->isChecked() && modeValue != 4)
            {
                writeBE32(process_, battlePtr_ + kOffGameState, 4);
                Sleep(kModeResetPulseMs);
            }
            const bool ok = writeAndVerifyU32(battlePtr_ + kOffGameState, modeValue);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("Game mode: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (writeHpCheckbox_->isChecked())
        {
            const bool ok = writeAndVerifyU32(battlePtr_ + kOffUiFlags, hpValue);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("HP/UI: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (writeInfiniteRoundCheckbox_->isChecked())
        {
            const bool ok = writeAndVerifyU32(battlePtr_ + kOffInfiniteRound, infiniteValue);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("Infinite round: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (roundTimerCheckbox_->isChecked())
        {
            const bool ok = writeAndVerifyU32(battlePtr_ + kOffRoundTimer, timerTicks);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("Round timer: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (p1ControllerCheckbox_->isChecked())
        {
            const bool ok = writeAndVerifyU32(kAddrP1State, 0);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("P1 state: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (p2CpuCheckbox_->isChecked())
        {
            const bool ok = writeAndVerifyU32(kAddrP2State, 1);
            allOk = allOk && ok;
            appendCheck(QStringLiteral("P2 state: %1").arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (countersCheckbox_->isChecked())
        {
            const bool c1ok = writeAndVerifyU32(battlePtr_ + kOffCounterP1, static_cast<std::uint32_t>(p1CounterSpin_->value()));
            const bool c2ok = writeAndVerifyU32(battlePtr_ + kOffCounterP2, static_cast<std::uint32_t>(p2CounterSpin_->value()));
            allOk = allOk && c1ok && c2ok;
            appendCheck(QStringLiteral("Counters: %1").arg((c1ok && c2ok) ? QStringLiteral("OK") : QStringLiteral("FAIL")));
        }

        if (!allOk)
        {
            statusLabel_->setText(QStringLiteral("Status: write/verify failures"));
            QMessageBox::warning(this, QStringLiteral("TRR Qt Trainer"), QStringLiteral("One or more values failed verification.\n\n%1").arg(checks));
            return false;
        }

        runtimeStabilizeCyclesLeft_ = stabilizeCheckbox_->isChecked() ? kStabilizeCycles : 0;
        runtimeLockEnabled_ = lockCheckbox_->isChecked();
        stageLockArmed_ = true;
        timer_->start();

        statusLabel_->setText(QStringLiteral("Status: applied and verified"));
        showTransientInfoMessage(QStringLiteral("TRR Qt Trainer"), QStringLiteral("Applied and verified.\n\n%1").arg(checks));
        return true;
    }

private:
    void checkInternetConnection()
    {
        if (connectionCheckInProgress_ || networkManager_ == nullptr)
        {
            return;
        }

        connectionCheckInProgress_ = true;
        connectionStatusLabel_->setText(QStringLiteral("Connection: Checking..."));

        QNetworkRequest request(QUrl(QStringLiteral("http://www.msftconnecttest.com/connecttest.txt")));
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = networkManager_->get(request);

        QTimer::singleShot(4000, reply, [reply]() {
            if (reply->isRunning())
            {
                reply->abort();
            }
        });

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const bool reachable = reply->error() == QNetworkReply::NoError &&
                                   reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200 &&
                                   reply->readAll().trimmed() == QByteArrayLiteral("Microsoft Connect Test");
            connectionStatusLabel_->setText(reachable
                                                ? QStringLiteral("Connection Status: Online")
                                                : QStringLiteral("Connection Status: Offline"));
            connectionCheckInProgress_ = false;
            reply->deleteLater();
        });
    }

    HANDLE process_ = nullptr;
    std::uint64_t battlePtr_ = 0;
    QString rpcs3ExePath_;
    QString rpcs3LatestExePath_;
    QString rpcs3CustomExePath_;
    QString cheatEngine72Path_;
    QString cheatEngine75Path_;
    QString rpcs3Build_ = QStringLiteral("0.0.13");
    QString rpcs3LatestBuild_ = QStringLiteral("0.0.00");
    QString rpcs3CustomBuild_ = QStringLiteral("0.0.00");
    int rpcs3ActiveExeChoice_ = 0;
    int npeb01406Rpcs3Choice_ = 0;
    int npub31250Rpcs3Choice_ = 1;
    int npjb00404Rpcs3Choice_ = 1;
    QString rpcs3GamePath_;
    QString lastLaunchedTitleId_;
    QString npeb01406GamePath_;
    QString npub31250GamePath_;
    QString npjb00404GamePath_;
    Rpcs3SessionController rpcs3Session_;

    QLabel* statusLabel_ = nullptr;
    QLabel* pointerLabel_ = nullptr;
    QLabel* connectionStatusLabel_ = nullptr;
    QLabel* runningGameLabel_ = nullptr;
    QLabel* runningBuildLabel_ = nullptr;

    QComboBox* p1Combo_ = nullptr;
    QComboBox* p2Combo_ = nullptr;
    QComboBox* stageCombo_ = nullptr;

    QCheckBox* lockCheckbox_ = nullptr;
    QCheckBox* lockSelectionCheckbox_ = nullptr;
    QCheckBox* autoDisableStageLockCheckbox_ = nullptr;
    QCheckBox* guardPauseCheckbox_ = nullptr;
    QCheckBox* stabilizeCheckbox_ = nullptr;
    QCheckBox* modeResetPulseCheckbox_ = nullptr;
    QCheckBox* writeModeCheckbox_ = nullptr;
    QCheckBox* writeHpCheckbox_ = nullptr;
    QCheckBox* hpPresetCheckbox_ = nullptr;
    QCheckBox* hpRandomCheckbox_ = nullptr;
    QCheckBox* writeInfiniteRoundCheckbox_ = nullptr;
    QCheckBox* p1ControllerCheckbox_ = nullptr;
    QCheckBox* p2CpuCheckbox_ = nullptr;
    QCheckBox* roundTimerCheckbox_ = nullptr;
    QCheckBox* countersCheckbox_ = nullptr;
    QCheckBox* advancedP1PositionCheckbox_ = nullptr;
    QCheckBox* advancedP2PositionCheckbox_ = nullptr;
    QCheckBox* advancedAnimationCheckbox_ = nullptr;
    QCheckBox* advancedGlobalStageCheckbox_ = nullptr;

    QSpinBox* infiniteRoundSpin_ = nullptr;
    QSpinBox* roundTimerSecondsSpin_ = nullptr;
    QSpinBox* p1CounterSpin_ = nullptr;
    QSpinBox* p2CounterSpin_ = nullptr;
    QSpinBox* guardPauseMsSpin_ = nullptr;
    QDoubleSpinBox* advancedP1XSpin_ = nullptr;
    QDoubleSpinBox* advancedP1YSpin_ = nullptr;
    QDoubleSpinBox* advancedP1ZSpin_ = nullptr;
    QDoubleSpinBox* advancedP2XSpin_ = nullptr;
    QDoubleSpinBox* advancedP2YSpin_ = nullptr;
    QDoubleSpinBox* advancedP2ZSpin_ = nullptr;

    QLineEdit* hpEdit_ = nullptr;
    QLineEdit* advancedAnimationEdit_ = nullptr;
    QLineEdit* advancedGlobalStageEdit_ = nullptr;
    QComboBox* modePresetCombo_ = nullptr;
    QComboBox* hpPresetCombo_ = nullptr;
    QComboBox* roundTimePresetCombo_ = nullptr;

    QPushButton* attachButton_ = nullptr;
    QPushButton* startRpcs3Button_ = nullptr;
    QPushButton* startGameButton_ = nullptr;
    QPushButton* restartGameButton_ = nullptr;
    QPushButton* resetEmulatorButton_ = nullptr;
    QPushButton* terminateRpcs3Button_ = nullptr;
    QPushButton* rpcs3ConfigButton_ = nullptr;
    QPushButton* snapshotButton_ = nullptr;
    QPushButton* trManualButton_ = nullptr;
    QPushButton* e3Button_ = nullptr;
    QPushButton* showLogsButton_ = nullptr;
    QPushButton* tutorialButton_ = nullptr;
    QPushButton* refreshButton_ = nullptr;
    QPushButton* applyButton_ = nullptr;
    QPushButton* readLiveButton_ = nullptr;
    QPushButton* stopLockButton_ = nullptr;
    QPushButton* profileConservativeButton_ = nullptr;
    QPushButton* profileBalancedButton_ = nullptr;
    QPushButton* profileAggressiveButton_ = nullptr;
    QPushButton* savePresetButton_ = nullptr;
    QPushButton* loadPresetButton_ = nullptr;
    QPushButton* advancedReadButton_ = nullptr;
    QPushButton* advancedApplyButton_ = nullptr;
    QPushButton* advancedMemoryButton_ = nullptr;
    QPushButton* runtimeButton_ = nullptr;
    QPushButton* valueWritesButton_ = nullptr;

    QLabel* monitorP1Id_ = nullptr;
    QLabel* monitorP2Id_ = nullptr;
    QLabel* monitorStage_ = nullptr;
    QLabel* monitorState_ = nullptr;
    QLabel* monitorTimer_ = nullptr;
    QLabel* monitorCounters_ = nullptr;
    QLabel* monitorUi_ = nullptr;
    QLabel* monitorInf_ = nullptr;
    QLabel* monitorGuard_ = nullptr;
    QLabel* advancedGameStateLabel_ = nullptr;
    QLabel* advancedGameStateReadLabel_ = nullptr;

    bool stageLockArmed_ = true;
    int guardTicksRemaining_ = 0;
    std::optional<std::uint32_t> prevTimer_;
    std::optional<std::uint32_t> prevCounterP1_;
    std::optional<std::uint32_t> prevCounterP2_;
    int runtimeStabilizeCyclesLeft_ = 0;
    bool runtimeLockEnabled_ = false;

    QTimer* timer_ = nullptr;
    QTimer* connectionStatusTimer_ = nullptr;
    QNetworkAccessManager* networkManager_ = nullptr;
    bool connectionCheckInProgress_ = false;
    QDialog* runtimeDialog_ = nullptr;
    QDialog* valueWritesDialog_ = nullptr;
    QDialog* advancedMemoryDialog_ = nullptr;
    QDialog* tutorialDialog_ = nullptr;
    QTabWidget* tutorialTabs_ = nullptr;
    QTextBrowser* tutorialReadmeView_ = nullptr;
    QTextBrowser* tutorialAutomationView_ = nullptr;
    QLineEdit* tutorialSearchEdit_ = nullptr;
    QLineEdit* tutorialSectionFilterEdit_ = nullptr;
    QListWidget* tutorialSectionList_ = nullptr;
    QString tutorialReadmeMarkdown_;
    QString tutorialAutomationMarkdown_;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/trr-qt-trainer.png")));
    MainWindow w;
    w.show();
    return app.exec();
}
