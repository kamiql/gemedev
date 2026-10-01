#include <gemedev/Application.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
enum class Screen { Start, Playing, Pause };

constexpr float windowWidth = 1280.0f;
constexpr float windowHeight = 720.0f;
constexpr float shopLeft = 900.0f;
constexpr float shopWidth = windowWidth - shopLeft;
constexpr float cookieSize = 320.0f;
constexpr float cursorSize = 42.0f;
constexpr float clickAnimationSeconds = 0.34f;
constexpr int upgradesPerPage = 3;
constexpr int maxCursors = 132;

constexpr double baseOrbitSeconds = 10.0;
constexpr double minimumOrbitSeconds = 1.0;
constexpr double pi = 3.14159265358979323846;
constexpr double tau = 2.0 * pi;
constexpr double firstRebirthCookies = 10'000'000.0;
constexpr float goldenSize = 84.0f;
constexpr double goldenLifetime = 9.0;
constexpr double goldenRewardSeconds = 25.0;
constexpr double bonusPerRebirthPoint = 0.5;
constexpr std::int64_t maxCookies = std::numeric_limits<std::int64_t>::max();
const double maxCookiesValue = static_cast<double>(maxCookies);

struct UpgradeStats {
    double baseClickValue = 1.0;
    double multiplier = 1.0;
    double rebirthMultiplier = 1.0;
    double cursorSpeedFactor = 1.0;
    int cursorCount = 0;

    double cookiesPerClick() const {
        return std::min(maxCookiesValue,
                        baseClickValue * multiplier * rebirthMultiplier);
    }
    double secondsPerOrbit() const {
        return baseOrbitSeconds / cursorSpeedFactor;
    }
    double cookiesPerSecond() const {
        return std::min(maxCookiesValue,
                        static_cast<double>(cursorCount) *
                        cookiesPerClick() / secondsPerOrbit());
    }
};

struct GameState {
    gd::Scene &scene;
    gd::TextureHandle cursorTexture;
    std::function<void()> manualClick;
    double orbitPhase = 0.0;
    std::vector<gd::Entity> cursors;
    std::vector<float> clickAnimationAge;
};

struct GoldenCookie {
    gd::Entity entity{};
    bool active = false;
    bool collected = false;
    double remaining = 0.0;
    double untilSpawn = 25.0;
};

struct OrbitSlot { int position; int capacity; float radius; };

std::string asset(const std::string &relative) {
    return (std::filesystem::path(GEMEDEV_ASSET_DIR) / relative).string();
}

std::filesystem::path persistentSaveFile() {
    std::filesystem::path directory;
#ifdef _WIN32
    if (const char *appData = std::getenv("APPDATA"); appData && *appData)
        directory = std::filesystem::path(appData) / "CookieClicker";
#else
    if (const char *xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg)
        directory = std::filesystem::path(xdg) / "cookie-clicker";
    if (directory.empty()) {
        if (const char *home = std::getenv("HOME"); home && *home)
            directory = std::filesystem::path(home) / ".local" / "share" /
                        "cookie-clicker";
    }
#endif
    if (directory.empty())
        directory = std::filesystem::temp_directory_path() / "cookie-clicker";
    std::filesystem::create_directories(directory);
    return directory / "cookie-clicker-save.json";
}

std::string formattedNumber(double value) {
    constexpr const char *suffixes[] = {"", "K", "M", "B", "T", "Qa", "Qi"};
    constexpr std::size_t lastSuffix = sizeof(suffixes) / sizeof(suffixes[0]) - 1;
    double scaled = std::abs(value);
    std::size_t suffix = 0;
    while (scaled >= 1000.0 && suffix < lastSuffix) {
        scaled /= 1000.0;
        ++suffix;
    }
    scaled = std::round(scaled * 100.0) / 100.0;
    if (scaled >= 1000.0 && suffix < lastSuffix) {
        scaled /= 1000.0;
        ++suffix;
    }
    if (value < 0.0) scaled = -scaled;
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(2) << scaled;
    std::string result = stream.str();
    const auto decimalPoint = result.find('.');
    if (decimalPoint != std::string::npos) {
        result.erase(result.find_last_not_of('0') + 1);
        if (result.back() == '.') result.pop_back();
        else result[decimalPoint] = ',';
    }
    return result + suffixes[suffix];
}

std::string formattedSeconds(double seconds) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(1) << seconds;
    return stream.str();
}

OrbitSlot orbitSlot(int index) {
    for (int ring = 0; ring < 6; ++ring) {
        const int capacity = 12 + ring * 4;
        if (index < capacity) return {index, capacity, 190.0f + ring * 26.0f};
        index -= capacity;
    }
    throw std::out_of_range("No cursor orbit slot available");
}

void updateCursorTransform(gd::Scene &scene, gd::Entity cursor, int index,
                           double orbitPhase, float clickAge) {
    auto *transform = scene.transform(cursor);
    const auto *cookie = scene.transform(scene.findEntity("cookie"));
    if (!transform || !cookie)
        throw std::runtime_error("Cursor or cookie has no Transform");

    const auto slot = orbitSlot(index);
    const double angle = tau * (orbitPhase +
                         static_cast<double>(slot.position) / slot.capacity);
    const float impulse = clickAge < clickAnimationSeconds
        ? std::sin(static_cast<float>(pi) * clickAge / clickAnimationSeconds)
        : 0.0f;
    const float radius = slot.radius - 26.0f * impulse;
    const float centerX = cookie->position.x + cookieSize / 2.0f;
    const float centerY = cookie->position.y + cookieSize / 2.0f;
    transform->position = {
        centerX + radius * static_cast<float>(std::cos(angle)) - cursorSize / 2.0f,
        centerY + radius * static_cast<float>(std::sin(angle)) - cursorSize / 2.0f
    };
    transform->rotationDegrees = static_cast<float>(angle * 180.0 / pi - 25.0);
}

gd::Entity createCursorEntity(GameState &game, int index) {
    const auto cursor = game.scene.createEntity("cursor_" + std::to_string(index));
    game.scene.add(cursor, gd::Transform{});
    game.scene.add(cursor, gd::Sprite{
        .size = {cursorSize, cursorSize},
        .texture = game.cursorTexture,
        .layer = 2
    });
    game.scene.onClick(cursor, game.manualClick);
    updateCursorTransform(game.scene, cursor, index, game.orbitPhase,
                          clickAnimationSeconds);
    return cursor;
}

class IUpgrade {
public:
    virtual ~IUpgrade() = default;
    virtual const std::string &title() const = 0;
    virtual double basePrice() const = 0;
    virtual std::string saveKey(const std::string &id) const = 0;
    virtual void contribute(UpgradeStats &stats, std::int64_t owned) const = 0;
    virtual std::string description(const UpgradeStats &stats) const = 0;
    virtual bool canPurchase(const UpgradeStats &stats) const = 0;
    virtual void onPurchase(GameState &game) const = 0;
};

class UpgradeBase : public IUpgrade {
public:
    UpgradeBase(std::string title, double basePrice)
        : title_(std::move(title)), basePrice_(basePrice) {}
    const std::string &title() const override { return title_; }
    double basePrice() const override { return basePrice_; }
    std::string saveKey(const std::string &id) const override {
        return "upgrade_" + id;
    }
    bool canPurchase(const UpgradeStats &) const override { return true; }
    void onPurchase(GameState &) const override {}
private:
    std::string title_;
    double basePrice_;
};

class CursorUpgrade final : public UpgradeBase {
public:
    CursorUpgrade(std::string title, double price, int perPurchase,
                  std::string existingSaveKey = {})
        : UpgradeBase(std::move(title), price),
          perPurchase_(perPurchase), existingSaveKey_(std::move(existingSaveKey)) {}
    std::string saveKey(const std::string &id) const override {
        return existingSaveKey_.empty() ? UpgradeBase::saveKey(id) : existingSaveKey_;
    }
    void contribute(UpgradeStats &stats, std::int64_t owned) const override {
        if (owned > (maxCursors - stats.cursorCount) / perPurchase_)
            throw std::runtime_error("Too many cursors in save file");
        stats.cursorCount += static_cast<int>(owned * perPurchase_);
    }
    std::string description(const UpgradeStats &stats) const override {
        return std::to_string(perPurchase_) + " / " +
               formattedSeconds(stats.secondsPerOrbit()) + " S";
    }
    bool canPurchase(const UpgradeStats &stats) const override {
        return stats.cursorCount <= maxCursors - perPurchase_;
    }
    void onPurchase(GameState &game) const override {
        for (int i = 0; i < perPurchase_; ++i) {
            game.cursors.push_back(createCursorEntity(
                game, static_cast<int>(game.cursors.size())));
            game.clickAnimationAge.push_back(clickAnimationSeconds);
        }
    }
private:
    int perPurchase_;
    std::string existingSaveKey_;
};

class AdditionUpgrade final : public UpgradeBase {
public:
    AdditionUpgrade(std::string title, double price, double amount)
        : UpgradeBase(std::move(title), price), amount_(amount) {}
    void contribute(UpgradeStats &stats, std::int64_t owned) const override {
        stats.baseClickValue = std::min(maxCookiesValue,
                              stats.baseClickValue + static_cast<double>(owned) * amount_);
    }
    std::string description(const UpgradeStats &) const override {
        return "+" + formattedNumber(amount_) + " / KLICK";
    }
private:
    double amount_;
};

class MultiplierUpgrade final : public UpgradeBase {
public:
    MultiplierUpgrade(std::string title, double price, double factor)
        : UpgradeBase(std::move(title), price), factor_(factor) {}
    void contribute(UpgradeStats &stats, std::int64_t owned) const override {
        stats.multiplier = std::min(maxCookiesValue,
            stats.multiplier * std::pow(factor_, static_cast<double>(owned)));
    }
    std::string description(const UpgradeStats &) const override {
        return "X" + formattedNumber(factor_) + " / KLICK";
    }
private:
    double factor_;
};

class CursorSpeedUpgrade final : public UpgradeBase {
public:
    CursorSpeedUpgrade(std::string title, double price, double percent)
        : UpgradeBase(std::move(title), price), percent_(percent) {}
    void contribute(UpgradeStats &stats, std::int64_t owned) const override {
        stats.cursorSpeedFactor = std::min(baseOrbitSeconds / minimumOrbitSeconds,
            stats.cursorSpeedFactor * std::pow(1.0 + percent_ / 100.0,
                                               static_cast<double>(owned)));
    }
    std::string description(const UpgradeStats &) const override {
        return "+" + formattedNumber(percent_) + "% TEMPO";
    }
    bool canPurchase(const UpgradeStats &stats) const override {
        return stats.secondsPerOrbit() > minimumOrbitSeconds + 0.000001;
    }
private:
    double percent_;
};

using UpgradeMap = std::map<std::string, std::unique_ptr<IUpgrade>>;
const UpgradeMap &upgrades() {
    static const UpgradeMap entries = [] {
        UpgradeMap result;
        result.emplace("01_cursor", std::make_unique<CursorUpgrade>(
            "CURSOR", 15.0, 1, "cursor_count"));
        result.emplace("02_click_1", std::make_unique<AdditionUpgrade>(
            "KLICK +1", 25.0, 1.0));
        result.emplace("03_click_5", std::make_unique<AdditionUpgrade>(
            "BOOST +5", 150.0, 5.0));
        result.emplace("04_click_25", std::make_unique<AdditionUpgrade>(
            "MEGA +25", 1000.0, 25.0));
        result.emplace("05_multiplier", std::make_unique<MultiplierUpgrade>(
            "MULTI X1.2", 400.0, 1.1));
        result.emplace("06_cursor_speed", std::make_unique<CursorSpeedUpgrade>(
            "CURSOR-TEMPO", 200.0, 2.5));
        return result;
    }();
    return entries;
}

double nonnegativeDouble(const gd::Scene &scene, const std::string &key) {
    const auto *value = scene.value(key);
    if (!value) return 0.0;
    double result;
    if (const auto *v = std::get_if<double>(value)) result = *v;
    else if (const auto *v = std::get_if<std::int64_t>(value))
        result = static_cast<double>(*v);
    else throw std::runtime_error("Invalid value: " + key);
    if (!std::isfinite(result) || result < 0.0 || result > maxCookiesValue)
        throw std::runtime_error("Invalid value: " + key);
    return result;
}

std::int64_t nonnegativeInt(const gd::Scene &scene, const std::string &key) {
    const auto *value = scene.value(key);
    if (!value) return 0;
    const auto *v = std::get_if<std::int64_t>(value);
    if (!v || *v < 0) throw std::runtime_error("Invalid value: " + key);
    return *v;
}

double cookieCount(const gd::Scene &scene) {
    return nonnegativeDouble(scene, "cookies");
}
std::int64_t rebirthPoints(const gd::Scene &scene) {
    return nonnegativeInt(scene, "rebirth_points");
}
std::int64_t rebirthCount(const gd::Scene &scene) {
    return nonnegativeInt(scene, "rebirth_count");
}

double rebirthGoal(std::int64_t level) {
    const long double target = static_cast<long double>(level) *
                               static_cast<long double>(level) * firstRebirthCookies;
    return target >= maxCookiesValue ? maxCookiesValue : static_cast<double>(target);
}
std::int64_t earnedRebirthPoints(double lifetime) {
    return static_cast<std::int64_t>(
        std::floor(std::sqrt(lifetime / firstRebirthCookies)));
}
std::int64_t pendingRebirthPoints(const gd::Scene &scene) {
    return std::max<std::int64_t>(0,
        earnedRebirthPoints(nonnegativeDouble(scene, "rebirth_lifetime")) -
        rebirthPoints(scene));
}
double rebirthBonus(std::int64_t points) {
    return 1.0 + bonusPerRebirthPoint * static_cast<double>(points);
}

std::int64_t upgradeCount(const gd::Scene &scene, const std::string &id,
                          const IUpgrade &upgrade) {
    return nonnegativeInt(scene, upgrade.saveKey(id));
}

UpgradeStats calculateStats(const gd::Scene &scene) {
    UpgradeStats stats;
    for (const auto &[id, upgrade] : upgrades())
        upgrade->contribute(stats, upgradeCount(scene, id, *upgrade));
    stats.rebirthMultiplier = rebirthBonus(rebirthPoints(scene));
    return stats;
}

double savedOrbitPhase(const gd::Scene &scene) {
    if (const auto *value = scene.value("cursor_orbit_phase")) {
        const auto *phase = std::get_if<double>(value);
        if (!phase || !std::isfinite(*phase))
            throw std::runtime_error("Invalid orbit phase");
        double result = std::fmod(*phase, 1.0);
        if (result < 0.0) result += 1.0;
        return result;
    }
    if (const auto *value = scene.value("cursor_orbit_time")) {
        const auto *time = std::get_if<double>(value);
        if (!time || !std::isfinite(*time))
            throw std::runtime_error("Invalid orbit time");
        double result = std::fmod(*time / baseOrbitSeconds, 1.0);
        if (result < 0.0) result += 1.0;
        return result;
    }
    return 0.0;
}

double upgradePrice(double basePrice, std::int64_t owned) {
    const double price = basePrice * std::pow(1.35, static_cast<double>(owned));
    if (!std::isfinite(price) || price >= maxCookiesValue) return maxCookiesValue;
    return std::ceil(price * 100.0) / 100.0;
}

void grantCookies(gd::Scene &scene, double amount) {
    if (!std::isfinite(amount) || amount <= 0.0) return;
    const double previous = cookieCount(scene);
    const double earned = std::min(amount, maxCookiesValue - previous);
    if (earned <= 0.0) return;
    scene.setValue("cookies", previous + earned);
    scene.setValue("rebirth_lifetime", std::min(maxCookiesValue,
        nonnegativeDouble(scene, "rebirth_lifetime") + earned));
    scene.setValue("rebirth_run", std::min(maxCookiesValue,
        nonnegativeDouble(scene, "rebirth_run") + earned));
}

void addClicks(gd::Scene &scene, std::int64_t clicks) {
    if (clicks <= 0) return;
    grantCookies(scene, static_cast<double>(clicks) *
                 calculateStats(scene).cookiesPerClick());
}

void animateCookie(gd::Scene &scene) {
    const auto cookie = scene.findEntity("cookie");
    if (!scene.valid(cookie) || !scene.transform(cookie)) return;
    scene.animate(cookie)
        .scaleTo({0.9f, 0.9f}, 0.075f, gd::Ease::Linear)
        .then()
        .scaleTo({1.0f, 1.0f}, 0.2f, gd::Ease::Linear);
}

void performRebirth(gd::Scene &scene, GameState &game) {
    const auto gain = pendingRebirthPoints(scene);
    if (gain <= 0) return;
    // The threshold is based on lifetime production, not current wallet balance.
    scene.setValue("rebirth_points", rebirthPoints(scene) + gain);
    scene.setValue("rebirth_count", rebirthCount(scene) + 1);
    scene.setValue("cookies", 0.0);
    scene.setValue("rebirth_run", 0.0);
    for (const auto &[id, upgrade] : upgrades())
        scene.setValue(upgrade->saveKey(id), std::int64_t{0});
    for (const auto cursor : game.cursors)
        if (scene.valid(cursor)) scene.destroyEntity(cursor);
    game.cursors.clear();
    game.clickAnimationAge.clear();
    game.orbitPhase = 0.0;
    scene.setValue("cursor_orbit_phase", 0.0);
    // "rebirth_lifetime" intentionally stays unchanged.
}
} // namespace

int main() {
    try {
        gd::Application app({
            .title = "Cookie Clicker",
            .width = static_cast<int>(windowWidth),
            .height = static_cast<int>(windowHeight)
        });
        const std::string savePath = persistentSaveFile().string();
        const std::string legacySavePath =
            (std::filesystem::temp_directory_path() / "cookie-clicker-save.json").string();
        auto font = app.assets().font(asset("fonts/Silkscreen-Regular.ttf"));
        auto cursorTexture = app.assets().texture(asset("textures/cursor.png"));
        auto &scene = app.scenes().create("main");
        scene.ui().setFont(font);
        auto goldenTexture = app.assets().texture(asset("textures/cookie.png"));
        auto background = app.assets().texture(asset("textures/background.png"));
        scene.setBackground(background);

        const auto cookie = scene.createEntity("cookie");
        scene.add(cookie, gd::Transform{
            .position = {(shopLeft - cookieSize) / 2.0f,
                         (windowHeight - cookieSize) / 2.0f}
        });
        scene.add(cookie, gd::Sprite{
            .size = {cookieSize, cookieSize},
            .texture = app.assets().texture(asset("textures/cookie.png"))
        });
        scene.setValue("cookies", 0.0);
        scene.setValue("cursor_count", std::int64_t{0});
        scene.setValue("cursor_orbit_phase", 0.0);
        if (std::filesystem::exists(savePath)) app.saves().load(scene, savePath);
        else if (std::filesystem::exists(legacySavePath))
            app.saves().load(scene, legacySavePath);

        const auto loadedCookie = scene.findEntity("cookie");
        if (!scene.valid(loadedCookie) || !scene.transform(loadedCookie))
            throw std::runtime_error("Cookie missing from scene");
        scene.transform(loadedCookie)->position = {
            (shopLeft - cookieSize) / 2.0f,
            (windowHeight - cookieSize) / 2.0f
        };

        // Old saves do not contain a lifetime ledger: use their current wallet as
        // the minimum known production. Previously spent cookies cannot be restored.
        const double wallet = cookieCount(scene);
        const double run = std::max(wallet, nonnegativeDouble(scene, "rebirth_run"));
        const double lifetime = std::max(run,
            nonnegativeDouble(scene, "rebirth_lifetime"));
        scene.setValue("rebirth_run", run);
        scene.setValue("rebirth_lifetime", lifetime);
        scene.setValue("rebirth_points", rebirthPoints(scene));
        scene.setValue("rebirth_count", rebirthCount(scene));
        if (rebirthPoints(scene) > earnedRebirthPoints(lifetime))
            throw std::runtime_error("Invalid rebirth points in save file");

        const auto loadedStats = calculateStats(scene);
        const double loadedPhase = savedOrbitPhase(scene);
        for (int i = 0; i < maxCursors; ++i) {
            const auto old = scene.findEntity("cursor_" + std::to_string(i));
            if (scene.valid(old)) scene.destroyEntity(old);
        }

        Screen screen = Screen::Start;
        std::optional<Screen> requestedScreen;
        int shopPage = 0;
        bool rebirthConfirm = false;
        scene.setPaused(true);
        const std::function<void()> manualClick = [&scene, &screen] {
            if (screen != Screen::Playing) return;
            addClicks(scene, 1);
            animateCookie(scene);
        };
        scene.onClick(loadedCookie, manualClick);
        GameState game{scene, cursorTexture, manualClick, loadedPhase, {}, {}};
        game.cursors.reserve(maxCursors);
        game.clickAnimationAge.reserve(maxCursors);
        for (int i = 0; i < loadedStats.cursorCount; ++i) {
            game.cursors.push_back(createCursorEntity(game, i));
            game.clickAnimationAge.push_back(clickAnimationSeconds);
        }
        scene.setValue("cursor_orbit_phase", game.orbitPhase);

        GoldenCookie golden;
        std::mt19937 random(std::random_device{}());
        std::uniform_real_distribution<float> spawnX(35.0f, shopLeft - goldenSize - 35.0f);
        std::uniform_real_distribution<float> spawnY(105.0f, windowHeight - goldenSize - 90.0f);
        std::uniform_real_distribution<double> spawnDelay(35.0, 60.0);
        // Lifetime is never reset by rebirth; the bonus uses current production.
        const auto spawnGolden = [&] {
            float x = 40.0f, y = 110.0f;
            for (int attempt = 0; attempt < 40; ++attempt) {
                x = spawnX(random);
                y = spawnY(random);
                const float cx = x + goldenSize / 2.0f;
                const float cy = y + goldenSize / 2.0f;
                const float dx = cx - shopLeft / 2.0f;
                const float dy = cy - windowHeight / 2.0f;
                if (dx * dx + dy * dy > 250.0f * 250.0f) break;
            }
            golden.entity = scene.createEntity("golden_cookie");
            scene.add(golden.entity, gd::Transform{.position = {x, y}});
            scene.add(golden.entity, gd::Sprite{
                .size = {goldenSize, goldenSize},
                .texture = goldenTexture, .layer = 3
            });
            golden.active = true;
            golden.collected = false;
            golden.remaining = goldenLifetime;
            scene.onClick(golden.entity, [&scene, &screen, &golden] {
                if (screen != Screen::Playing || !golden.active || golden.collected)
                    return;
                golden.collected = true;
                const auto stats = calculateStats(scene);
                // 25 seconds of current cursor production, at least 50 manual clicks.
                const double reward = std::max(
                    50.0 * stats.cookiesPerClick(),
                    goldenRewardSeconds * stats.cookiesPerSecond());
                grantCookies(scene, reward);
                animateCookie(scene);
            });
        };

        std::function<void()> rebuildMenu;
        rebuildMenu = [&app, &scene, &screen, &requestedScreen,
                       &shopPage, &rebirthConfirm, &game,
                       &rebuildMenu, &savePath] {
            scene.ui().clear();
            if (screen != Screen::Playing) {
                const auto panel = scene.ui().panel(
                    {{440.0f, 435.0f}, {400.0f, 90.0f}});
                scene.ui().button(
                    screen == Screen::Start ? "SPIEL STARTEN" : "FORTSETZEN",
                    {{20.0f, 16.0f}, {360.0f, 58.0f}},
                    [&requestedScreen] { requestedScreen = Screen::Playing; },
                    panel);
                return;
            }

            const auto shop = scene.ui().panel(
                {{shopLeft, 0.0f}, {shopWidth, windowHeight}},
                0, {0.13f, 0.17f, 0.22f, 1.0f});
            const int upgradePages = static_cast<int>(
                (upgrades().size() + upgradesPerPage - 1) / upgradesPerPage);
            const int pageCount = upgradePages + 1;
            shopPage = std::clamp(shopPage, 0, pageCount - 1);
            const bool rebirthPage = shopPage == upgradePages;
            const auto stats = calculateStats(scene);
            scene.ui().label(rebirthPage ? "REBIRTH" : "UPGRADES",
                             {20.0f, 22.0f}, shop);
            scene.ui().label(rebirthPage
                ? "BONUS X" + formattedNumber(stats.rebirthMultiplier)
                : "KLICK: " + formattedNumber(stats.cookiesPerClick()),
                {20.0f, 53.0f}, shop);

            if (rebirthPage) {
                const auto panel = scene.ui().panel(
                    {{14.0f, 97.0f}, {shopWidth - 28.0f, 480.0f}}, shop,
                    {0.19f, 0.24f, 0.30f, 1.0f});
                const auto points = rebirthPoints(scene);
                const auto pending = pendingRebirthPoints(scene);
                const double lifetime = nonnegativeDouble(scene, "rebirth_lifetime");
                const double nextGoal = rebirthGoal(points + pending + 1);
                const double missing = std::max(0.0, nextGoal - lifetime);
                scene.ui().label("PUNKTE: " + formattedNumber(static_cast<double>(points)),
                                 {12.0f, 18.0f}, panel);
                scene.ui().label("REBIRTHS: " + std::to_string(rebirthCount(scene)),
                                 {12.0f, 52.0f}, panel);
                scene.ui().label("RUN: " + formattedNumber(
                    nonnegativeDouble(scene, "rebirth_run")), {12.0f, 86.0f}, panel);
                scene.ui().label("TOTAL: " + formattedNumber(lifetime),
                                 {12.0f, 120.0f}, panel);
                scene.ui().label("NEXT +1: " + formattedNumber(nextGoal),
                                 {12.0f, 154.0f}, panel);
                scene.ui().label("NOCH +1: " + formattedNumber(missing),
                                 {12.0f, 188.0f}, panel);
                scene.ui().label("GEWINN: +" + std::to_string(pending),
                                 {12.0f, 222.0f}, panel);
                scene.ui().label("NEU: X" + formattedNumber(
                    rebirthBonus(points + pending)), {12.0f, 256.0f}, panel);
                scene.ui().label("RESET: COOKIES+UPGRADES",
                                 {12.0f, 306.0f}, panel);
                if (pending > 0) {
                    scene.ui().button(rebirthConfirm ? "RESET BESTAETIGEN"
                                                    : "REBIRTH +" + std::to_string(pending),
                        {{12.0f, 354.0f}, {shopWidth - 52.0f, 52.0f}},
                        [&app, &scene, &game, &shopPage, &rebirthConfirm,
                         &rebuildMenu, &savePath, &screen] {
                            if (screen != Screen::Playing ||
                                pendingRebirthPoints(scene) <= 0) return;
                            if (!rebirthConfirm) {
                                rebirthConfirm = true;
                                rebuildMenu();
                                return;
                            }
                            performRebirth(scene, game);
                            rebirthConfirm = false;
                            shopPage = 0;
                            app.saves().save(scene, savePath);
                            rebuildMenu();
                        }, panel);
                    if (rebirthConfirm) {
                        scene.ui().button("ABBRECHEN",
                            {{12.0f, 420.0f}, {shopWidth - 52.0f, 46.0f}},
                            [&rebirthConfirm, &rebuildMenu] {
                                rebirthConfirm = false;
                                rebuildMenu();
                            }, panel);
                    }
                } else {
                    scene.ui().label("ERST ZIEL ERREICHEN",
                                     {12.0f, 366.0f}, panel);
                }
            } else {
                int index = 0;
                for (const auto &[id, upgrade] : upgrades()) {
                    if (index < shopPage * upgradesPerPage ||
                        index >= (shopPage + 1) * upgradesPerPage) {
                        ++index;
                        continue;
                    }
                    const int row = index % upgradesPerPage;
                    const float y = 97.0f + row * 165.0f;
                    const auto panel = scene.ui().panel(
                        {{14.0f, y}, {shopWidth - 28.0f, 150.0f}},
                        shop, {0.19f, 0.24f, 0.30f, 1.0f});
                    const auto owned = upgradeCount(scene, id, *upgrade);
                    const double price = upgradePrice(upgrade->basePrice(), owned);
                    const bool unavailable = !upgrade->canPurchase(stats) ||
                        price >= maxCookiesValue ||
                        owned == std::numeric_limits<std::int64_t>::max();
                    scene.ui().label(upgrade->title(), {12.0f, 12.0f}, panel);
                    scene.ui().label("x" + std::to_string(owned),
                                     {shopWidth - 85.0f, 12.0f}, panel);
                    scene.ui().label(upgrade->description(stats),
                                     {12.0f, 49.0f}, panel);
                    scene.ui().button(unavailable ? "MAXIMUM"
                        : "KAUFEN: " + formattedNumber(price),
                        {{12.0f, 91.0f}, {shopWidth - 52.0f, 47.0f}},
                        [&scene, &screen, &game, &rebuildMenu, id] {
                            if (screen != Screen::Playing) return;
                            const auto &selected = *upgrades().at(id);
                            const auto owned = upgradeCount(scene, id, selected);
                            const auto stats = calculateStats(scene);
                            const double price = upgradePrice(selected.basePrice(), owned);
                            if (!selected.canPurchase(stats) || price >= maxCookiesValue ||
                                owned == std::numeric_limits<std::int64_t>::max() ||
                                cookieCount(scene) < price) return;
                            selected.onPurchase(game);
                            scene.setValue("cookies", std::max(0.0,
                                cookieCount(scene) - price));
                            scene.setValue(selected.saveKey(id), owned + 1);
                            rebuildMenu();
                        }, panel);
                    ++index;
                }
            }

            scene.ui().label(std::to_string(shopPage + 1) + "/" +
                             std::to_string(pageCount),
                             {shopWidth / 2.0f - 25.0f, 604.0f}, shop);
            if (shopPage > 0) {
                scene.ui().button("<", {{20.0f, 643.0f}, {155.0f, 49.0f}},
                    [&shopPage, &rebirthConfirm, &rebuildMenu] {
                        --shopPage;
                        rebirthConfirm = false;
                        rebuildMenu();
                    }, shop);
            }
            if (shopPage + 1 < pageCount) {
                scene.ui().button(">",
                    {{shopWidth - 175.0f, 643.0f}, {155.0f, 49.0f}},
                    [&shopPage, &rebirthConfirm, &rebuildMenu] {
                        ++shopPage;
                        rebirthConfirm = false;
                        rebuildMenu();
                    }, shop);
            }
        };

        scene.setOverlay([&scene, &screen, &golden, font](gd::Canvas &canvas) {
            const auto centeredText = [&canvas, font](
                float centerX, float y, const std::string &text, gd::Color color) {
                canvas.text({centerX - font->textWidth(text) / 2.0f, y},
                            text, font, color);
            };
            if (screen == Screen::Playing) {
                const auto *transform = scene.transform(scene.findEntity("cookie"));
                if (transform) {
                    const float centerX = transform->position.x + cookieSize / 2.0f;
                    const auto stats = calculateStats(scene);
                    centeredText(centerX, transform->position.y - 55.0f,
                        "COOKIES: " + formattedNumber(cookieCount(scene)),
                        {1.0f, 0.94f, 0.78f, 1.0f});
                    centeredText(centerX, transform->position.y + cookieSize + 25.0f,
                        "PRO SEKUNDE: " + formattedNumber(stats.cookiesPerSecond()),
                        {0.82f, 0.89f, 0.96f, 1.0f});
                }
                canvas.text({20.0f, 20.0f}, "ESC: PAUSE", font,
                            {0.75f, 0.80f, 0.87f, 1.0f});
                if (golden.active && !golden.collected) {
                    canvas.text({20.0f, 90.0f}, "GOLDENER COOKIE: KLICKEN!",
                                font, {1.0f, 0.85f, 0.53f, 1.0f});
                }
                canvas.text({20.0f, 54.0f},
                    "REBIRTH X" + formattedNumber(rebirthBonus(rebirthPoints(scene))),
                    font, {0.82f, 0.89f, 0.96f, 1.0f});
                return;
            }
            canvas.rect({{0.0f, 0.0f}, {windowWidth, windowHeight}},
                        {0.03f, 0.04f, 0.07f, 0.78f});
            canvas.rect({{400.0f, 180.0f}, {480.0f, 370.0f}},
                        {0.13f, 0.17f, 0.22f, 0.98f});
            centeredText(windowWidth / 2.0f, 225.0f,
                screen == Screen::Start ? "COOKIE CLICKER" : "PAUSE",
                {1.0f, 0.85f, 0.53f, 1.0f});
            centeredText(windowWidth / 2.0f, 325.0f,
                "COOKIES: " + formattedNumber(cookieCount(scene)),
                {0.82f, 0.89f, 0.96f, 1.0f});
        });

        rebuildMenu();
        scene.setUpdateCallback(
            [&app, &scene, &screen, &requestedScreen, &shopPage,
             &rebuildMenu, &game, &golden, &random, &spawnDelay, &spawnGolden,
             savePath, lastSavedInterval = 0LL, lastRebirthUiSecond = -1LL]
            (gd::Scene &current, const gd::Input &input, float dt) mutable {
                if (input.keyPressed(gd::Key::Escape)) {
                    if (screen == Screen::Playing) requestedScreen = Screen::Pause;
                    else if (screen == Screen::Pause) {
                        app.quit();
                        return;
                    }
                }
                if (requestedScreen) {
                    screen = *requestedScreen;
                    requestedScreen.reset();
                    scene.setPaused(screen != Screen::Playing);
                    rebuildMenu();
                }
                if (screen == Screen::Playing && dt > 0.0f) {
                    const double secondsPerOrbit = calculateStats(scene).secondsPerOrbit();
                    const double nextPhase = game.orbitPhase +
                                             static_cast<double>(dt) / secondsPerOrbit;
                    std::int64_t cursorClicks = 0;
                    for (std::size_t i = 0; i < game.cursors.size(); ++i) {
                        game.clickAnimationAge[i] = std::min(clickAnimationSeconds,
                            game.clickAnimationAge[i] + dt);
                        const auto slot = orbitSlot(static_cast<int>(i));
                        const double offset = static_cast<double>(slot.position) / slot.capacity;
                        const auto previousLap = static_cast<long long>(
                            std::floor(game.orbitPhase + offset));
                        const auto nextLap = static_cast<long long>(
                            std::floor(nextPhase + offset));
                        if (nextLap > previousLap) {
                            cursorClicks += nextLap - previousLap;
                            game.clickAnimationAge[i] = static_cast<float>(
                                (nextPhase + offset - nextLap) * secondsPerOrbit);
                        }
                        updateCursorTransform(scene, game.cursors[i],
                            static_cast<int>(i), nextPhase, game.clickAnimationAge[i]);
                    }
                    game.orbitPhase = std::fmod(nextPhase, 1.0);
                    scene.setValue("cursor_orbit_phase", game.orbitPhase);
                    if (cursorClicks > 0) {
                        addClicks(scene, cursorClicks);
                        animateCookie(scene);
                    }
                }
                if (screen == Screen::Playing && dt > 0.0f) {
                    if (golden.active) {
                        golden.remaining -= dt;
                        if (golden.collected || golden.remaining <= 0.0) {
                            if (scene.valid(golden.entity))
                                scene.destroyEntity(golden.entity);
                            golden.active = false;
                            golden.collected = false;
                            golden.untilSpawn = spawnDelay(random);
                        }
                    } else {
                        golden.untilSpawn -= dt;
                        if (golden.untilSpawn <= 0.0) spawnGolden();
                    }
                }
                const auto uiSecond = static_cast<long long>(app.runtimeSeconds());
                const int upgradePages = static_cast<int>(
                    (upgrades().size() + upgradesPerPage - 1) / upgradesPerPage);
                if (screen == Screen::Playing && shopPage == upgradePages &&
                    uiSecond > lastRebirthUiSecond) {
                    lastRebirthUiSecond = uiSecond;
                    rebuildMenu();
                }
                const auto interval = static_cast<long long>(
                    app.runtimeSeconds() / 30.0);
                if (interval > lastSavedInterval) {
                    app.saves().save(current, savePath);
                    lastSavedInterval = interval;
                }
            });
        app.run();
        app.saves().save(scene, savePath);
    } catch (const std::exception &error) {
        std::cerr << "Cookie Clicker: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
