#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <wincodec.h>
#include <mmsystem.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "winmm.lib")

// ==================== 配置 ====================
const int SCREEN_W = 100;
const int SCREEN_H = 30;
const int PIXEL_W = SCREEN_W;
const int PIXEL_H = SCREEN_H * 2;

const int MAP_W = 24;
const int MAP_H = 24;

const double MOVE_SPEED = 0.055;
const double ROT_SPEED = 0.07;
const double PLAYER_RAD = 0.20;

const int    MAX_ALLY = 4;
const double ALLY_SPEED = 0.035;
const int    ALLY_HP = 5;
const int    ALLY_ATTACK_CD = 40;
const double ALLY_SIGHT = 8.0;
const double ALLY_FOLLOW_DIST = 3.0;

const int RADAR_SIZE = 11;

// ==================== 音效 ====================
enum SoundID {
    SND_SHOOT_PISTOL = 0,
    SND_SHOOT_RIFLE,
    SND_SHOOT_SNIPER,
    SND_SHOOT_SHOTGUN,
    SND_RELOAD,
    SND_HIT,
    SND_KILL,
    SND_HURT,
    SND_GRENADE_THROW,
    SND_GRENADE_BOOM,
    SND_LEVEL_UP,
    SND_ACHIEVEMENT,
    SND_GAME_OVER,
    SND_MENU_CLICK
};

struct SoundDef { int freq; int durMs; };
SoundDef SOUNDS[] = {
    { 800, 30 }, { 600, 25 }, { 1000, 50 }, { 400, 60 },
    { 500, 80 }, { 1200, 20 }, { 1500, 60 }, { 200, 100 },
    { 700, 40 }, { 150, 300 }, { 1000, 100 }, { 1800, 80 },
    { 100, 500 }, { 900, 20 }
};
int SOUND_COUNT = sizeof(SOUNDS) / sizeof(SOUNDS[0]);
bool soundEnabled = true;

void playSound(int id) {
    if (!soundEnabled) return;
    if (id < 0 || id >= SOUND_COUNT) return;
    Beep(SOUNDS[id].freq, SOUNDS[id].durMs);
}

// ==================== 难度 ====================
enum Difficulty { DIFF_EASY = 0, DIFF_NORMAL = 1, DIFF_HELL = 2 };
struct DiffConfig {
    int enemyDmg;
    int attackCd;
    int spawnInterval;
    int maxEnemies;
    int startHp;
    const wchar_t* name;
};
DiffConfig DIFFS[3] = {
    { 8,  30, 100, 8,  120, L"\u7B80\u5355" },
    { 15, 22, 70,  14, 100, L"\u666E\u901A" },
    { 25, 14, 40,  20, 60,  L"\u5730\u72F1" }
};

// ==================== 敌人类型 ====================
enum EnemyType {
    ET_NORMAL = 0, ET_FAST, ET_TANK, ET_SNIPER,
    ET_BOSS, ET_HEALER, ET_GHOST
};

// ==================== 武器 ====================
enum WeaponType { WT_PISTOL = 0, WT_RIFLE, WT_SNIPER, WT_SHOTGUN };
struct WeaponDef {
    int magSize; int shootCd; int reloadTime;
    double spread; int pellets; int dmgPerPellet;
    double range; int soundId; const wchar_t* name;
};
WeaponDef WEAPONS[4] = {
    { 12, 6, 40, 0.03, 1, 34, 12.0, SND_SHOOT_PISTOL,  L"\u624B\u67AA" },
    { 30, 4, 50, 0.05, 1, 25, 10.0, SND_SHOOT_RIFLE,   L"\u6B65\u67AA" },
    { 5,  30, 80, 0.00, 1, 100, 20.0, SND_SHOOT_SNIPER, L"\u72D9\u51FB\u67AA" },
    { 8,  20, 60, 0.15, 6, 15, 6.0,  SND_SHOOT_SHOTGUN, L"\u6563\u5F39\u67AA" }
};

// ==================== 任务数据 ====================
const int TOTAL_MISSIONS = 100;

struct MissionDef {
    const wchar_t* title;
    const wchar_t* target;
    int enemyHpBonus;
    int maxEnemies;
    int spawnInterval;
};

MissionDef MISSIONS[TOTAL_MISSIONS] = {
    // ===== 第 1-10 关 =====
    { L"\u4EFB\u52A1 01 - \u524D\u54E8\u9632\u7EBF",       L"\u76EE\u6807\uFF1A\u6E05\u7406\u524D\u54E8\u533A\u57DF\u3002",                    0, 8,  100 },
    { L"\u4EFB\u52A1 02 - \u8857\u9053\u6E05\u626B",         L"\u76EE\u6807\uFF1A\u6D88\u706D\u8857\u9053\u4E0A\u7684\u6E38\u8361\u654C\u519B\u3002",          0, 8,  95  },
    { L"\u4EFB\u52A1 03 - \u4ED3\u5E93\u7A81\u56F4",         L"\u76EE\u6807\uFF1A\u7A81\u7834\u654C\u4EBA\u5728\u4ED3\u5E93\u7684\u5C01\u9501\u3002",          1, 9,  90  },
    { L"\u4EFB\u52A1 04 - \u5730\u4E0B\u901A\u9053",         L"\u76EE\u6807\uFF1A\u6E05\u7406\u5730\u4E0B\u901A\u9053\u7684\u4F0F\u51FB\u3002",              1, 9,  88  },
    { L"\u4EFB\u52A1 05 - \u9996\u9886\u51FA\u73B0",         L"\u76EE\u6807\uFF1A\u51FB\u6740\u7B2C\u4E00\u4E2A\u654C\u4EBA\u9996\u9886\u3002",              1, 10, 85  },
    { L"\u4EFB\u52A1 06 - \u5E9F\u589F\u641C\u7D22",         L"\u76EE\u6807\uFF1A\u641C\u7D22\u5E9F\u589F\u4E2D\u7684\u6B8B\u4F59\u654C\u519B\u3002",          2, 10, 82  },
    { L"\u4EFB\u52A1 07 - \u6865\u6881\u5B88\u536B",         L"\u76EE\u6807\uFF1A\u5B88\u4F4F\u6865\u6881\uFF0C\u963B\u6B62\u654C\u4EBA\u901A\u8FC7\u3002",      2, 11, 80  },
    { L"\u4EFB\u52A1 08 - \u533B\u7597\u7AD9\u7A81\u88AD",     L"\u76EE\u6807\uFF1A\u62FF\u4E0B\u654C\u4EBA\u533B\u7597\u7AD9\u3002",                  2, 11, 78  },
    { L"\u4EFB\u52A1 09 - \u9690\u8EAB\u5371\u673A",         L"\u76EE\u6807\uFF1A\u5E94\u5BF9\u654C\u4EBA\u9690\u8EAB\u5355\u4F4D\u3002",                2, 12, 75  },
    { L"\u4EFB\u52A1 10 - \u7B2C\u4E8C\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u5F3A\u5316\u9996\u9886\u3002",                    3, 12, 72  },
    // ===== 第 11-20 关 =====
    { L"\u4EFB\u52A1 11 - \u96F7\u8FBE\u7AD9",             L"\u76EE\u6807\uFF1A\u5360\u9886\u654C\u4EBA\u96F7\u8FBE\u7AD9\u3002",                  3, 13, 70  },
    { L"\u4EFB\u52A1 12 - \u591C\u95F4\u884C\u52A8",         L"\u76EE\u6807\uFF1A\u591C\u95F4\u6E05\u7406\u654C\u4EBA\u54E8\u6240\u3002",                3, 13, 68  },
    { L"\u4EFB\u52A1 13 - \u6CBB\u7597\u5175\u5DE5\u5382",     L"\u76EE\u6807\uFF1A\u6467\u6BC1\u654C\u4EBA\u6CBB\u7597\u5175\u5DE5\u5382\u3002",              3, 14, 66  },
    { L"\u4EFB\u52A1 14 - \u88C5\u7532\u8F66\u961F",         L"\u76EE\u6807\uFF1A\u62E6\u622A\u654C\u4EBA\u88C5\u7532\u8F66\u961F\u3002",              4, 14, 64  },
    { L"\u4EFB\u52A1 15 - \u7B2C\u4E09\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u91CD\u88C5\u9996\u9886\u3002",                  4, 15, 62  },
    { L"\u4EFB\u52A1 16 - \u7A7A\u964D\u533A",             L"\u76EE\u6807\uFF1A\u9632\u5B88\u7A7A\u964D\u533A\u3002",                      4, 15, 60  },
    { L"\u4EFB\u52A1 17 - \u6863\u6848\u5BA4",             L"\u76EE\u6807\uFF1A\u62A2\u5360\u654C\u4EBA\u6863\u6848\u5BA4\u3002",                  4, 16, 58  },
    { L"\u4EFB\u52A1 18 - \u5316\u5B66\u5DE5\u5382",         L"\u76EE\u6807\uFF1A\u6E05\u7406\u5316\u5B66\u5DE5\u5382\u533A\u57DF\u3002",              5, 16, 56  },
    { L"\u4EFB\u52A1 19 - \u5E94\u6025\u901A\u9053",         L"\u76EE\u6807\uFF1A\u6253\u5F00\u5E94\u6025\u901A\u9053\u3002",                  5, 17, 54  },
    { L"\u4EFB\u52A1 20 - \u7B2C\u56DB\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u53CC\u91CD\u9996\u9886\u3002",                  5, 17, 52  },
    // ===== 第 21-30 关 =====
    { L"\u4EFB\u52A1 21 - \u5929\u53F0\u9632\u5FA1",         L"\u76EE\u6807\uFF1A\u5B88\u4F4F\u5929\u53F0\u3002",                      6, 18, 50  },
    { L"\u4EFB\u52A1 22 - \u6DF1\u5165\u654C\u540E",         L"\u76EE\u6807\uFF1A\u6DF1\u5165\u654C\u4EBA\u540E\u65B9\u3002",                  6, 18, 48  },
    { L"\u4EFB\u52A1 23 - \u4FE1\u53F7\u5E72\u6270",         L"\u76EE\u6807\uFF1A\u7834\u574F\u654C\u4EBA\u4FE1\u53F7\u5854\u3002",              6, 19, 46  },
    { L"\u4EFB\u52A1 24 - \u533B\u9662\u9632\u7EBF",         L"\u76EE\u6807\uFF1A\u9632\u5B88\u533B\u9662\uFF0C\u4FDD\u62A4\u5E73\u6C11\u3002",          7, 19, 44  },
    { L"\u4EFB\u52A1 25 - \u7B2C\u4E94\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u9690\u8EAB\u9996\u9886\u3002",                  7, 20, 42  },
    { L"\u4EFB\u52A1 26 - \u6838\u5FC3\u533A\u57DF",         L"\u76EE\u6807\uFF1A\u7A81\u5165\u654C\u4EBA\u6838\u5FC3\u533A\u57DF\u3002",              7, 20, 40  },
    { L"\u4EFB\u52A1 27 - \u6700\u540E\u7684\u9632\u7EBF",     L"\u76EE\u6807\uFF1A\u5B88\u4F4F\u6700\u540E\u4E00\u9053\u9632\u7EBF\u3002",              8, 21, 38  },
    { L"\u4EFB\u52A1 28 - \u654C\u4EBA\u6307\u6325\u90E8",     L"\u76EE\u6807\uFF1A\u6467\u6BC1\u654C\u4EBA\u6307\u6325\u90E8\u3002",              8, 22, 36  },
    { L"\u4EFB\u52A1 29 - \u6700\u540E\u7684\u6323\u624E",     L"\u76EE\u6807\uFF1A\u6E05\u7406\u6B8B\u4F59\u654C\u519B\u3002",                  9, 23, 34  },
    { L"\u4EFB\u52A1 30 - \u6700\u7EC8\u51B3\u6218",         L"\u76EE\u6807\uFF1A\u51FB\u6740\u6700\u7EC8\u9996\u9886\uFF0C\u7ED3\u675F\u6218\u4E89\u3002",      10, 25, 32  },
    // ===== 第 31-40 关 =====
    { L"\u4EFB\u52A1 31 - \u6B7B\u4EA1\u8D70\u5ECA",         L"\u76EE\u6807\uFF1A\u7A81\u7834\u6B7B\u4EA1\u8D70\u5ECA\u3002",                  10, 25, 30  },
    { L"\u4EFB\u52A1 32 - \u57CE\u5E02\u5E9F\u589F",         L"\u76EE\u6807\uFF1A\u6E05\u7406\u57CE\u5E02\u5E9F\u589F\u3002",                  11, 26, 30  },
    { L"\u4EFB\u52A1 33 - \u65E0\u4EBA\u533A",             L"\u76EE\u6807\uFF1A\u63A2\u7D22\u65E0\u4EBA\u533A\u3002",                      11, 26, 28  },
    { L"\u4EFB\u52A1 34 - \u96F7\u533A",                 L"\u76EE\u6807\uFF1A\u7A81\u7834\u96F7\u533A\u3002",                        11, 27, 28  },
    { L"\u4EFB\u52A1 35 - \u7B2C\u516D\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u53CC\u91CD\u9690\u8EAB\u9996\u9886\u3002",          12, 27, 26  },
    { L"\u4EFB\u52A1 36 - \u5317\u6781\u57FA\u5730",         L"\u76EE\u6807\uFF1A\u5360\u9886\u5317\u6781\u57FA\u5730\u3002",                  12, 28, 26  },
    { L"\u4EFB\u52A1 37 - \u6C99\u6F20\u98CE\u66B4",         L"\u76EE\u6807\uFF1A\u5728\u6C99\u6F20\u98CE\u66B4\u4E2D\u751F\u5B58\u3002",              12, 28, 24  },
    { L"\u4EFB\u52A1 38 - \u6DF1\u6D77\u57FA\u5730",         L"\u76EE\u6807\uFF1A\u6E05\u7406\u6DF1\u6D77\u57FA\u5730\u3002",                  13, 29, 24  },
    { L"\u4EFB\u52A1 39 - \u592A\u7A7A\u7AD9",             L"\u76EE\u6807\uFF1A\u62FF\u4E0B\u592A\u7A7A\u7AD9\u3002",                      13, 29, 22  },
    { L"\u4EFB\u52A1 40 - \u7B2C\u4E03\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u53D8\u5F02\u9996\u9886\u3002",                  14, 30, 22  },
    // ===== 第 41-50 关 =====
    { L"\u4EFB\u52A1 41 - \u71C3\u70E7\u57CE\u5E02",         L"\u76EE\u6807\uFF1A\u5728\u71C3\u70E7\u7684\u57CE\u5E02\u4E2D\u751F\u5B58\u3002",          14, 30, 20  },
    { L"\u4EFB\u52A1 42 - \u6838\u7206\u540E",             L"\u76EE\u6807\uFF1A\u6838\u7206\u540E\u7684\u5E9F\u589F\u4E2D\u524D\u884C\u3002",              14, 31, 20  },
    { L"\u4EFB\u52A1 43 - \u57FA\u56E0\u5B9E\u9A8C\u5BA4",     L"\u76EE\u6807\uFF1A\u6467\u6BC1\u57FA\u56E0\u5B9E\u9A8C\u5BA4\u3002",                15, 31, 19  },
    { L"\u4EFB\u52A1 44 - \u65F6\u95F4\u88C2\u7F1D",         L"\u76EE\u6807\uFF1A\u7A33\u5B9A\u65F6\u95F4\u88C2\u7F1D\u3002",                  15, 32, 19  },
    { L"\u4EFB\u52A1 45 - \u7B2C\u516B\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u65F6\u95F4\u9996\u9886\u3002",                  15, 32, 18  },
    { L"\u4EFB\u52A1 46 - \u53CD\u7269\u8D28\u4E16\u754C",     L"\u76EE\u6807\uFF1A\u5728\u53CD\u7269\u8D28\u4E16\u754C\u4E2D\u751F\u5B58\u3002",          16, 33, 18  },
    { L"\u4EFB\u52A1 47 - \u9ED1\u6D1E\u8FB9\u7F18",         L"\u76EE\u6807\uFF1A\u5728\u9ED1\u6D1E\u8FB9\u7F18\u6218\u6597\u3002",              16, 33, 17  },
    { L"\u4EFB\u52A1 48 - \u5E73\u884C\u5B87\u5B99",         L"\u76EE\u6807\uFF1A\u6E05\u7406\u5E73\u884C\u5B87\u5B99\u7684\u654C\u519B\u3002",          16, 34, 17  },
    { L"\u4EFB\u52A1 49 - \u6700\u540E\u7684\u57CE\u5E02",     L"\u76EE\u6807\uFF1A\u5B88\u4F4F\u6700\u540E\u7684\u57CE\u5E02\u3002",              17, 34, 16  },
    { L"\u4EFB\u52A1 50 - \u7B2C\u4E5D\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u5B87\u5B99\u9996\u9886\u3002",                  17, 35, 16  },
    // ===== 第 51-60 关 =====
    { L"\u4EFB\u52A1 51 - \u9F99\u5DE2",                 L"\u76EE\u6807\uFF1A\u6E05\u7406\u9F99\u5DE2\u3002",                        18, 35, 15  },
    { L"\u4EFB\u52A1 52 - \u51B0\u5C01\u738B\u5EA7",         L"\u76EE\u6807\uFF1A\u653B\u5360\u51B0\u5C01\u738B\u5EA7\u3002",                  18, 36, 15  },
    { L"\u4EFB\u52A1 53 - \u706B\u7130\u5C71",             L"\u76EE\u6807\uFF1A\u5728\u706B\u7130\u5C71\u4E2D\u751F\u5B58\u3002",                18, 36, 14  },
    { L"\u4EFB\u52A1 54 - \u6DF1\u6E0A",                 L"\u76EE\u6807\uFF1A\u63A2\u7D22\u6DF1\u6E0A\u3002",                        19, 37, 14  },
    { L"\u4EFB\u52A1 55 - \u7B2C\u5341\u4E2A\u9996\u9886",     L"\u76EE\u6807\uFF1A\u51FB\u6740\u53E4\u4EE3\u9996\u9886\u3002",                  19, 37, 13  },
    { L"\u4EFB\u52A1 56 - \u6D6E\u7A7A\u57CE",             L"\u76EE\u6807\uFF1A\u62FF\u4E0B\u6D6E\u7A7A\u57CE\u3002",                      19, 38, 13  },
    { L"\u4EFB\u52A1 57 - \u6C34\u4E0B\u57CE",             L"\u76EE\u6807\uFF1A\u6E05\u7406\u6C34\u4E0B\u57CE\u3002",                      20, 38, 13  },
    { L"\u4EFB\u52A1 58 - \u53E4\u4EE3\u9057\u8FF9",         L"\u76EE\u6807\uFF1A\u63A2\u7D22\u53E4\u4EE3\u9057\u8FF9\u3002",                  20, 39, 12  },
    { L"\u4EFB\u52A1 59 - \u661F\u9645\u95E8",             L"\u76EE\u6807\uFF1A\u7A81\u7834\u661F\u9645\u95E8\u3002",                      20, 39, 12  },
    { L"\u4EFB\u52A1 60 - \u7B2C\u5341\u4E00\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u661F\u9645\u9996\u9886\u3002",                21, 40, 12  },
    // ===== 第 61-70 关 =====
    { L"\u4EFB\u52A1 61 - \u65F6\u5149\u56DE\u5ECA",         L"\u76EE\u6807\uFF1A\u7A33\u5B9A\u65F6\u5149\u56DE\u5ECA\u3002",                  21, 40, 11  },
    { L"\u4EFB\u52A1 62 - \u5E7B\u5F71\u57CE",             L"\u76EE\u6807\uFF1A\u6E05\u7406\u5E7B\u5F71\u57CE\u3002",                      21, 41, 11  },
    { L"\u4EFB\u52A1 63 - \u865A\u65E0\u4E4B\u5730",         L"\u76EE\u6807\uFF1A\u5728\u865A\u65E0\u4E4B\u5730\u751F\u5B58\u3002",              22, 41, 11  },
    { L"\u4EFB\u52A1 64 - \u661F\u4E91\u5E26",             L"\u76EE\u6807\uFF1A\u7A81\u7834\u661F\u4E91\u5E26\u3002",                      22, 42, 10  },
    { L"\u4EFB\u52A1 65 - \u7B2C\u5341\u4E8C\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u5E7B\u5F71\u9996\u9886\u3002",                22, 42, 10  },
    { L"\u4EFB\u52A1 66 - \u91CD\u529B\u5F02\u5E38",         L"\u76EE\u6807\uFF1A\u5728\u91CD\u529B\u5F02\u5E38\u533A\u751F\u5B58\u3002",          23, 43, 10  },
    { L"\u4EFB\u52A1 67 - \u5149\u901F\u98DE\u8239",         L"\u76EE\u6807\uFF1A\u62E6\u622A\u5149\u901F\u98DE\u8239\u3002",                  23, 43, 9   },
    { L"\u4EFB\u52A1 68 - \u9ED1\u6D1E\u5185\u90E8",         L"\u76EE\u6807\uFF1A\u5728\u9ED1\u6D1E\u5185\u90E8\u751F\u5B58\u3002",              23, 44, 9   },
    { L"\u4EFB\u52A1 69 - \u5B87\u5B99\u8FB9\u7F18",         L"\u76EE\u6807\uFF1A\u5230\u8FBE\u5B87\u5B99\u8FB9\u7F18\u3002",                  24, 44, 9   },
    { L"\u4EFB\u52A1 70 - \u7B2C\u5341\u4E09\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u5B87\u5B99\u8FB9\u7F18\u9996\u9886\u3002",            24, 45, 8   },
    // ===== 第 71-80 关 =====
    { L"\u4EFB\u52A1 71 - \u795E\u754C\u4E4B\u95E8",         L"\u76EE\u6807\uFF1A\u6253\u5F00\u795E\u754C\u4E4B\u95E8\u3002",                  24, 45, 8   },
    { L"\u4EFB\u52A1 72 - \u5929\u4F7F\u964D\u4E34",         L"\u76EE\u6807\uFF1A\u5E94\u5BF9\u5929\u4F7F\u964D\u4E34\u3002",                  25, 46, 8   },
    { L"\u4EFB\u52A1 73 - \u6076\u9B54\u57CE",             L"\u76EE\u6807\uFF1A\u6E05\u7406\u6076\u9B54\u57CE\u3002",                      25, 46, 8   },
    { L"\u4EFB\u52A1 74 - \u51A5\u754C\u6DF1\u5904",         L"\u76EE\u6807\uFF1A\u63A2\u7D22\u51A5\u754C\u6DF1\u5904\u3002",                  25, 47, 7   },
    { L"\u4EFB\u52A1 75 - \u7B2C\u5341\u56DB\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u6076\u9B54\u9996\u9886\u3002",                26, 47, 7   },
    { L"\u4EFB\u52A1 76 - \u4E5D\u91CD\u5730\u72F1",         L"\u76EE\u6807\uFF1A\u7A81\u7834\u4E5D\u91CD\u5730\u72F1\u3002",                  26, 48, 7   },
    { L"\u4EFB\u52A1 77 - \u5929\u5802\u4E4B\u8DEF",         L"\u76EE\u6807\uFF1A\u5BFB\u627E\u5929\u5802\u4E4B\u8DEF\u3002",                  26, 48, 6   },
    { L"\u4EFB\u52A1 78 - \u521B\u4E16\u795E",             L"\u76EE\u6807\uFF1A\u5E94\u5BF9\u521B\u4E16\u795E\u3002",                      27, 49, 6   },
    { L"\u4EFB\u52A1 79 - \u5B87\u5B99\u7EC8\u7ED3",         L"\u76EE\u6807\uFF1A\u963B\u6B62\u5B87\u5B99\u7EC8\u7ED3\u3002",                  27, 49, 6   },
    { L"\u4EFB\u52A1 80 - \u7B2C\u5341\u4E94\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u521B\u4E16\u795E\u3002",                  27, 50, 6   },
    // ===== 第 81-90 关 =====
    { L"\u4EFB\u52A1 81 - \u65E0\u9650\u5FAA\u73AF",         L"\u76EE\u6807\uFF1A\u6253\u7834\u65E0\u9650\u5FAA\u73AF\u3002",                  28, 50, 5   },
    { L"\u4EFB\u52A1 82 - \u56DB\u7EF4\u7A7A\u95F4",         L"\u76EE\u6807\uFF1A\u5728\u56DB\u7EF4\u7A7A\u95F4\u4E2D\u751F\u5B58\u3002",          28, 51, 5   },
    { L"\u4EFB\u52A1 83 - \u91CF\u5B50\u9886\u57DF",         L"\u76EE\u6807\uFF1A\u63A2\u7D22\u91CF\u5B50\u9886\u57DF\u3002",                  28, 51, 5   },
    { L"\u4EFB\u52A1 84 - \u53CD\u5411\u5B87\u5B99",         L"\u76EE\u6807\uFF1A\u62A2\u6551\u53CD\u5411\u5B87\u5B99\u3002",                  29, 52, 5   },
    { L"\u4EFB\u52A1 85 - \u7B2C\u5341\u516D\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u91CF\u5B50\u9996\u9886\u3002",                29, 52, 4   },
    { L"\u4EFB\u52A1 86 - \u591A\u91CD\u5B87\u5B99",         L"\u76EE\u6807\uFF1A\u7A33\u5B9A\u591A\u91CD\u5B87\u5B99\u3002",                  29, 53, 4   },
    { L"\u4EFB\u52A1 87 - \u65F6\u95F4\u7EC8\u70B9",         L"\u76EE\u6807\uFF1A\u62B5\u8FBE\u65F6\u95F4\u7EC8\u70B9\u3002",                  30, 53, 4   },
    { L"\u4EFB\u52A1 88 - \u7A7A\u95F4\u8FB9\u754C",         L"\u76EE\u6807\uFF1A\u5B88\u4F4F\u7A7A\u95F4\u8FB9\u754C\u3002",                  30, 54, 4   },
    { L"\u4EFB\u52A1 89 - \u5E73\u884C\u4E16\u754C\u5927\u6218", L"\u76EE\u6807\uFF1A\u8D62\u5F97\u5E73\u884C\u4E16\u754C\u5927\u6218\u3002",          30, 55, 3   },
    { L"\u4EFB\u52A1 90 - \u7B2C\u5341\u4E03\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u5E73\u884C\u9996\u9886\u3002",                31, 56, 3   },
    // ===== 第 91-100 关 =====
    { L"\u4EFB\u52A1 91 - \u521B\u4E16\u4E4B\u521D",         L"\u76EE\u6807\uFF1A\u56DE\u5230\u521B\u4E16\u4E4B\u521D\u3002",                  32, 57, 3   },
    { L"\u4EFB\u52A1 92 - \u5B87\u5B99\u6838\u5FC3",         L"\u76EE\u6807\uFF1A\u7A81\u5165\u5B87\u5B99\u6838\u5FC3\u3002",                  33, 58, 3   },
    { L"\u4EFB\u52A1 93 - \u6700\u540E\u7684\u5149",         L"\u76EE\u6807\uFF1A\u5BFB\u627E\u6700\u540E\u7684\u5149\u3002",                  34, 59, 3   },
    { L"\u4EFB\u52A1 94 - \u9ED1\u6697\u7EDF\u6CBB",         L"\u76EE\u6807\uFF1A\u963B\u6B62\u9ED1\u6697\u7EDF\u6CBB\u3002",                  35, 60, 3   },
    { L"\u4EFB\u52A1 95 - \u7B2C\u5341\u516B\u4E2A\u9996\u9886", L"\u76EE\u6807\uFF1A\u51FB\u6740\u9ED1\u6697\u9996\u9886\u3002",                36, 61, 2   },
    { L"\u4EFB\u52A1 96 - \u5149\u660E\u4E4B\u8DEF",         L"\u76EE\u6807\uFF1A\u8E0F\u4E0A\u5149\u660E\u4E4B\u8DEF\u3002",                  37, 62, 2   },
    { L"\u4EFB\u52A1 97 - \u65B0\u751F",                 L"\u76EE\u6807\uFF1A\u5E26\u6765\u65B0\u751F\u3002",                        38, 63, 2   },
    { L"\u4EFB\u52A1 98 - \u7EC8\u6781\u5BF9\u51B3",         L"\u76EE\u6807\uFF1A\u4E0E\u6700\u7EC8\u654C\u4EBA\u51B3\u6218\u3002",              39, 64, 2   },
    { L"\u4EFB\u52A1 99 - \u6700\u540E\u7684\u58EB\u5175",     L"\u76EE\u6807\uFF1A\u6210\u4E3A\u6700\u540E\u7684\u58EB\u5175\u3002",              40, 65, 2   },
    { L"\u4EFB\u52A1 100 - \u7EC8\u7ED3",                 L"\u76EE\u6807\uFF1A\u7ED3\u675F\u8FD9\u573A\u6218\u4E89\uFF0C\u56DE\u5BB6\u3002",          50, 70, 1   }
};

// 任务情报
const wchar_t* MISSION_BRIEFS[TOTAL_MISSIONS] = {
    L"\u60C5\u62A5\uFF1A\u654C\u519B\u524D\u54E8\u90E8\u961F\u6B63\u5728\u96C6\u7ED3\u3002",
    L"\u60C5\u62A5\uFF1A\u8857\u9053\u4E0A\u5230\u5904\u90FD\u662F\u654C\u4EBA\u54E8\u5175\u3002",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u628A\u4ED3\u5E93\u6539\u9020\u6210\u4E86\u524D\u7EBF\u57FA\u5730\u3002",
    L"\u60C5\u62A5\uFF1A\u5730\u4E0B\u901A\u9053\u57CB\u4F0F\u4E86\u5927\u91CF\u654C\u519B\u3002",
    L"\u60C5\u62A5\uFF1A\u9996\u9886\u51FA\u73B0\uFF0C\u8B66\u60D5\uFF01",
    L"\u60C5\u62A5\uFF1A\u5E9F\u589F\u4E2D\u6709\u654C\u4EBA\u6B8B\u90E8\u6D3B\u52A8\u3002",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u8BD5\u56FE\u7A81\u7834\u6865\u6881\u9632\u7EBF\u3002",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u533B\u7597\u7AD9\u6B63\u5728\u6CBB\u7597\u4F24\u5458\u3002",
    L"\u60C5\u62A5\uFF1A\u9690\u8EAB\u5355\u4F4D\u6B63\u5728\u6F5C\u4F0F\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u4E8C\u4E2A\u9996\u9886\u6BD4\u7B2C\u4E00\u4E2A\u66F4\u5F3A\u3002",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u96F7\u8FBE\u7AD9\u662F\u4ED6\u4EEC\u7684\u773C\u775B\u3002",
    L"\u60C5\u62A5\uFF1A\u591C\u95F4\u884C\u52A8\uFF0C\u6CE8\u610F\u9690\u853D\u3002",
    L"\u60C5\u62A5\uFF1A\u6CBB\u7597\u5175\u5DE5\u5382\u5728\u6E90\u6E90\u4E0D\u65AD\u751F\u4EA7\u3002",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u88C5\u7532\u8F66\u961F\u6B63\u5728\u8FD0\u9001\u7269\u8D44\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u4E09\u4E2A\u9996\u9886\u5DF2\u88C5\u5907\u91CD\u88C5\u3002",
    L"\u60C5\u62A5\uFF1A\u7A7A\u964D\u533A\u662F\u654C\u4EBA\u7684\u8865\u7ED9\u70B9\u3002",
    L"\u60C5\u62A5\uFF1A\u6863\u6848\u5BA4\u85CF\u7740\u654C\u4EBA\u7684\u60C5\u62A5\u3002",
    L"\u60C5\u62A5\uFF1A\u5316\u5B66\u5DE5\u5382\u6709\u6BD2\u6C14\uFF0C\u5C0F\u5FC3\u3002",
    L"\u60C5\u62A5\uFF1A\u5E94\u6025\u901A\u9053\u662F\u64A4\u79BB\u7684\u552F\u4E00\u51FA\u8DEF\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u56DB\u4E2A\u9996\u9886\u6709\u4E24\u4E2A\u3002",
    L"\u60C5\u62A5\uFF1A\u5929\u53F0\u662F\u5236\u9AD8\u70B9\uFF0C\u6613\u5B88\u96BE\u653B\u3002",
    L"\u60C5\u62A5\uFF1A\u6DF1\u5165\u654C\u540E\uFF0C\u6CA1\u6709\u63F4\u519B\u3002",
    L"\u60C5\u62A5\uFF1A\u4FE1\u53F7\u5854\u662F\u654C\u4EBA\u7684\u901A\u8BAF\u67A2\u7EBD\u3002",
    L"\u60C5\u62A5\uFF1A\u533B\u9662\u6709\u5E73\u6C11\uFF0C\u4E0D\u80FD\u8BEF\u4F24\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u4E94\u4E2A\u9996\u9886\u4F1A\u9690\u8EAB\u3002",
    L"\u60C5\u62A5\uFF1A\u6838\u5FC3\u533A\u57DF\u662F\u654C\u4EBA\u7684\u6700\u540E\u9632\u7EBF\u3002",
    L"\u60C5\u62A5\uFF1A\u6700\u540E\u4E00\u9053\u9632\u7EBF\uFF0C\u5B88\u4F4F\uFF01",
    L"\u60C5\u62A5\uFF1A\u654C\u4EBA\u6307\u6325\u90E8\u662F\u4ED6\u4EEC\u7684\u5927\u8111\u3002",
    L"\u60C5\u62A5\uFF1A\u6B8B\u4F59\u654C\u519B\u6B63\u5728\u6323\u624E\u3002",
    L"\u60C5\u62A5\uFF1A\u6700\u7EC8\u51B3\u6218\uFF0C\u6CA1\u6709\u9000\u8DEF\uFF01",
    L"\u60C5\u62A5\uFF1A\u6B7B\u4EA1\u8D70\u5ECA\u57CB\u4F0F\u7740\u65E0\u6570\u9690\u8EAB\u5175\u3002",
    L"\u60C5\u62A5\uFF1A\u57CE\u5E02\u5E9F\u589F\u4E2D\u6709\u5927\u91CF\u654C\u4EBA\u6B8B\u90E8\u3002",
    L"\u60C5\u62A5\uFF1A\u65E0\u4EBA\u533A\u662F\u654C\u4EBA\u7684\u5B9E\u9A8C\u573A\u3002",
    L"\u60C5\u62A5\uFF1A\u96F7\u533A\u6709\u5F3A\u78C1\u573A\uFF0C\u5C0F\u5FC3\u884C\u52A8\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u516D\u4E2A\u9996\u9886\u662F\u53CC\u91CD\u9690\u8EAB\u3002",
    L"\u60C5\u62A5\uFF1A\u5317\u6781\u57FA\u5730\u6C14\u6E29\u6781\u4F4E\u3002",
    L"\u60C5\u62A5\uFF1A\u6C99\u6F20\u98CE\u66B4\u4F1A\u5F71\u54CD\u89C6\u7EBF\u3002",
    L"\u60C5\u62A5\uFF1A\u6DF1\u6D77\u57FA\u5730\u6709\u6C34\u538B\u5371\u9669\u3002",
    L"\u60C5\u62A5\uFF1A\u592A\u7A7A\u7AD9\u5931\u91CD\u73AF\u5883\uFF0C\u884C\u52A8\u8C28\u614E\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u4E03\u4E2A\u9996\u9886\u5DF2\u53D8\u5F02\u3002",
    L"\u60C5\u62A5\uFF1A\u57CE\u5E02\u6B63\u5728\u71C3\u70E7\uFF0C\u901F\u6218\u901F\u51B3\u3002",
    L"\u60C5\u62A5\uFF1A\u6838\u7206\u540E\u8F90\u5C04\u6D53\u5EA6\u6781\u9AD8\u3002",
    L"\u60C5\u62A5\uFF1A\u57FA\u56E0\u5B9E\u9A8C\u5BA4\u6709\u751F\u5316\u6B66\u5668\u3002",
    L"\u60C5\u62A5\uFF1A\u65F6\u95F4\u88C2\u7F1D\u6B63\u5728\u6269\u5927\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u516B\u4E2A\u9996\u9886\u64CD\u63A7\u65F6\u95F4\u3002",
    L"\u60C5\u62A5\uFF1A\u53CD\u7269\u8D28\u4E16\u754C\u7269\u7406\u6CD5\u5219\u5931\u6548\u3002",
    L"\u60C5\u62A5\uFF1A\u9ED1\u6D1E\u8FB9\u7F18\u65F6\u95F4\u6D41\u901D\u4E0D\u540C\u3002",
    L"\u60C5\u62A5\uFF1A\u5E73\u884C\u5B87\u5B99\u6709\u65E0\u9650\u53EF\u80FD\u3002",
    L"\u60C5\u62A5\uFF1A\u6700\u540E\u7684\u57CE\u5E02\u662F\u4EBA\u7C7B\u6700\u540E\u5E0C\u671B\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u4E5D\u4E2A\u9996\u9886\u6765\u81EA\u5B87\u5B99\u6DF1\u5904\u3002",
    L"\u60C5\u62A5\uFF1A\u9F99\u5DE2\u4E2D\u6709\u53E4\u4EE3\u751F\u7269\u3002",
    L"\u60C5\u62A5\uFF1A\u51B0\u5C01\u738B\u5EA7\u85CF\u7740\u53E4\u4EE3\u79D8\u5BC6\u3002",
    L"\u60C5\u62A5\uFF1A\u706B\u7130\u5C71\u6B63\u5728\u55B7\u53D1\u3002",
    L"\u60C5\u62A5\uFF1A\u6DF1\u6E0A\u4E2D\u6709\u672A\u77E5\u751F\u7269\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E2A\u9996\u9886\u662F\u53E4\u4EE3\u5B88\u62A4\u8005\u3002",
    L"\u60C5\u62A5\uFF1A\u6D6E\u7A7A\u57CE\u5728\u4E91\u7AEF\u4E4B\u4E0A\u3002",
    L"\u60C5\u62A5\uFF1A\u6C34\u4E0B\u57CE\u6709\u5931\u843D\u7684\u6587\u660E\u3002",
    L"\u60C5\u62A5\uFF1A\u53E4\u4EE3\u9057\u8FF9\u57CB\u85CF\u7740\u771F\u76F8\u3002",
    L"\u60C5\u62A5\uFF1A\u661F\u9645\u95E8\u662F\u5B87\u5B99\u7684\u5165\u53E3\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E00\u4E2A\u9996\u9886\u6765\u81EA\u661F\u9645\u3002",
    L"\u60C5\u62A5\uFF1A\u65F6\u5149\u56DE\u5ECA\u8FDE\u63A5\u8FC7\u53BB\u672A\u6765\u3002",
    L"\u60C5\u62A5\uFF1A\u5E7B\u5F71\u57CE\u662F\u5E7B\u89C9\u7684\u4EA7\u7269\u3002",
    L"\u60C5\u62A5\uFF1A\u865A\u65E0\u4E4B\u5730\u4EC0\u4E48\u90FD\u6CA1\u6709\u3002",
    L"\u60C5\u62A5\uFF1A\u661F\u4E91\u5E26\u662F\u65B0\u7684\u6218\u573A\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E8C\u4E2A\u9996\u9886\u64CD\u63A7\u5E7B\u5F71\u3002",
    L"\u60C5\u62A5\uFF1A\u91CD\u529B\u5F02\u5E38\u533A\u884C\u52A8\u8270\u96BE\u3002",
    L"\u60C5\u62A5\uFF1A\u5149\u901F\u98DE\u8239\u901F\u5EA6\u6781\u5FEB\u3002",
    L"\u60C5\u62A5\uFF1A\u9ED1\u6D1E\u5185\u90E8\u65F6\u95F4\u505C\u6EDE\u3002",
    L"\u60C5\u62A5\uFF1A\u5B87\u5B99\u8FB9\u7F18\u662F\u672A\u77E5\u9886\u57DF\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E09\u4E2A\u9996\u9886\u5B88\u536B\u8FB9\u7F18\u3002",
    L"\u60C5\u62A5\uFF1A\u795E\u754C\u4E4B\u95E8\u8FDE\u63A5\u4E0D\u540C\u7EF4\u5EA6\u3002",
    L"\u60C5\u62A5\uFF1A\u5929\u4F7F\u964D\u4E34\u5E26\u6765\u672A\u77E5\u53D8\u6570\u3002",
    L"\u60C5\u62A5\uFF1A\u6076\u9B54\u57CE\u662F\u6076\u9B54\u7684\u5DE2\u7A74\u3002",
    L"\u60C5\u62A5\uFF1A\u51A5\u754C\u6DF1\u5904\u9634\u6C14\u6781\u91CD\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u56DB\u4E2A\u9996\u9886\u662F\u6076\u9B54\u9886\u4E3B\u3002",
    L"\u60C5\u62A5\uFF1A\u4E5D\u91CD\u5730\u72F1\u5C42\u5C42\u5371\u673A\u3002",
    L"\u60C5\u62A5\uFF1A\u5929\u5802\u4E4B\u8DEF\u5145\u6EE1\u8B66\u60D5\u3002",
    L"\u60C5\u62A5\uFF1A\u521B\u4E16\u795E\u62E5\u6709\u65E0\u9650\u529B\u91CF\u3002",
    L"\u60C5\u62A5\uFF1A\u5B87\u5B99\u7EC8\u7ED3\u8FD1\u5728\u773C\u524D\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E94\u4E2A\u9996\u9886\u662F\u521B\u4E16\u795E\u3002",
    L"\u60C5\u62A5\uFF1A\u65E0\u9650\u5FAA\u73AF\u56F0\u4F4F\u4E86\u6240\u6709\u4EBA\u3002",
    L"\u60C5\u62A5\uFF1A\u56DB\u7EF4\u7A7A\u95F4\u8D85\u51FA\u5E38\u8BC6\u3002",
    L"\u60C5\u62A5\uFF1A\u91CF\u5B50\u9886\u57DF\u5145\u6EE1\u4E0D\u786E\u5B9A\u6027\u3002",
    L"\u60C5\u62A5\uFF1A\u53CD\u5411\u5B87\u5B99\u6B63\u5728\u5D29\u6E83\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u516D\u4E2A\u9996\u9886\u638C\u63A7\u91CF\u5B50\u3002",
    L"\u60C5\u62A5\uFF1A\u591A\u91CD\u5B87\u5B99\u4EA4\u7EC7\u5728\u4E00\u8D77\u3002",
    L"\u60C5\u62A5\uFF1A\u65F6\u95F4\u7EC8\u70B9\u5C31\u5728\u524D\u65B9\u3002",
    L"\u60C5\u62A5\uFF1A\u7A7A\u95F4\u8FB9\u754C\u65E9\u5DF2\u7834\u788E\u3002",
    L"\u60C5\u62A5\uFF1A\u5E73\u884C\u4E16\u754C\u5927\u6218\u7206\u53D1\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u4E03\u4E2A\u9996\u9886\u6765\u81EA\u5E73\u884C\u4E16\u754C\u3002",
    L"\u60C5\u62A5\uFF1A\u521B\u4E16\u4E4B\u521D\u4E00\u5207\u672A\u5B9A\u3002",
    L"\u60C5\u62A5\uFF1A\u5B87\u5B99\u6838\u5FC3\u662F\u4E00\u5207\u7684\u8D77\u6E90\u3002",
    L"\u60C5\u62A5\uFF1A\u6700\u540E\u7684\u5149\u5C06\u51B3\u5B9A\u672A\u6765\u3002",
    L"\u60C5\u62A5\uFF1A\u9ED1\u6697\u7EDF\u6CBB\u4E86\u5927\u534A\u5B87\u5B99\u3002",
    L"\u60C5\u62A5\uFF1A\u7B2C\u5341\u516B\u4E2A\u9996\u9886\u662F\u9ED1\u6697\u4E4B\u738B\u3002",
    L"\u60C5\u62A5\uFF1A\u5149\u660E\u4E4B\u8DEF\u5C31\u5728\u811A\u4E0B\u3002",
    L"\u60C5\u62A5\uFF1A\u65B0\u751F\u5C31\u5728\u773C\u524D\u3002",
    L"\u60C5\u62A5\uFF1A\u7EC8\u6781\u5BF9\u51B3\u5373\u5C06\u5F00\u59CB\u3002",
    L"\u60C5\u62A5\uFF1A\u4F60\u662F\u6700\u540E\u7684\u58EB\u5175\u3002",
    L"\u60C5\u62A5\uFF1A\u7ED3\u675F\u8FD9\u573A\u6218\u4E89\uFF0C\u56DE\u5BB6\u3002"
};

// ==================== 成就 ====================
struct Achievement { bool unlocked; const wchar_t* name; const wchar_t* desc; };
Achievement ACHIEVEMENTS[] = {
    { false, L"\u521D\u6B21\u51FB\u6740", L"\u51FB\u6740 1 \u4E2A\u654C\u4EBA" },
    { false, L"\u767E\u4EBA\u65A9", L"\u7D2F\u8BA1\u51FB\u6740 100 \u4E2A\u654C\u4EBA" },
    { false, L"\u624B\u96F7\u5927\u5E08", L"\u7528\u624B\u96F7\u70B8\u6B7B 10 \u4E2A\u654C\u4EBA" },
    { false, L"\u65E0\u4F24\u901A\u5173", L"\u4E0D\u53D7\u4EFB\u4F55\u4F24\u5BB3\u901A\u8FC7\u4E00\u5173" },
    { false, L"\u5168\u6B66\u5668\u7CBE\u901A", L"\u4F7F\u7528\u8FC7\u6240\u6709 4 \u628A\u6B66\u5668" },
    { false, L"\u7206\u7834\u4E13\u5BB6", L"\u4E00\u6B21\u624B\u96F7\u70B8\u6B7B 3 \u4E2A\u654C\u4EBA" },
    { false, L"\u4E0D\u6B7B\u4E4B\u8EAB", L"\u5355\u5C40\u4E0D\u6B7B\u4EA1\u901A\u5173" }
};
const int ACHIEVEMENT_COUNT = sizeof(ACHIEVEMENTS) / sizeof(ACHIEVEMENTS[0]);

int statTotalKills = 0;
int statGrenadeKills = 0;
int statGrenadeMaxKills = 0;
bool weaponUsed[4] = { false, false, false, false };
bool tookDamageThisLevel = false;

void unlockAchievement(int idx) {
    if (idx < 0 || idx >= ACHIEVEMENT_COUNT) return;
    if (ACHIEVEMENTS[idx].unlocked) return;
    ACHIEVEMENTS[idx].unlocked = true;
    playSound(SND_ACHIEVEMENT);
}

// ==================== 排行榜 ====================
const int LEADERBOARD_MAX = 5;
struct ScoreEntry { int score; int kills; int difficulty; int level; };
ScoreEntry leaderboard[LEADERBOARD_MAX];
int leaderboardCount = 0;
const char* LEADERBOARD_FILE = "leaderboard.dat";

void loadLeaderboard() {
    std::ifstream f(LEADERBOARD_FILE, std::ios::binary);
    if (!f) { leaderboardCount = 0; return; }
    f.read((char*)&leaderboardCount, sizeof(leaderboardCount));
    if (leaderboardCount < 0 || leaderboardCount > LEADERBOARD_MAX) leaderboardCount = 0;
    f.read((char*)leaderboard, sizeof(ScoreEntry) * leaderboardCount);
}
void saveLeaderboard() {
    std::ofstream f(LEADERBOARD_FILE, std::ios::binary);
    if (!f) return;
    f.write((char*)&leaderboardCount, sizeof(leaderboardCount));
    f.write((char*)leaderboard, sizeof(ScoreEntry) * leaderboardCount);
}
void submitScore(int score, int kills, int difficulty, int level) {
    ScoreEntry entry = { score, kills, difficulty, level };
    int pos = leaderboardCount;
    for (int i = 0; i < leaderboardCount; i++) {
        if (score > leaderboard[i].score) { pos = i; break; }
    }
    if (pos >= LEADERBOARD_MAX) return;
    if (leaderboardCount < LEADERBOARD_MAX) leaderboardCount++;
    for (int i = leaderboardCount - 1; i > pos; i--) leaderboard[i] = leaderboard[i - 1];
    leaderboard[pos] = entry;
    saveLeaderboard();
}

// ==================== 游戏状态 ====================
enum GameState {
    GS_MAIN_MENU = 0, GS_DIFF_SELECT, GS_TUTORIAL, GS_PLAYING,
    GS_PAUSED, GS_GAME_OVER, GS_ACHIEVEMENTS, GS_LEADERBOARD,
    GS_LEVEL_COMPLETE, GS_LEVEL_BRIEF, GS_LEVEL_DIALOG, GS_MISSION_SELECT
};
GameState gameState = GS_MAIN_MENU;
Difficulty currentDiff = DIFF_NORMAL;
DiffConfig* diffCfg = &DIFFS[DIFF_NORMAL];

bool endlessMode = false;
int endlessWave = 0;
int missionSelectIndex = 0;

int tutorialPage = 0;
const int TUTORIAL_PAGES = 3;
int pauseMenuIndex = 0;

struct Settings {
    int showRadar = 1;
    int showCrosshair = 1;
    int showFps = 0;
    int soundOn = 1;
};
Settings settings;

// ==================== 精灵表 ====================
const wchar_t* TEX_URLS[6] = {
    L"https://i.postimg.cc/0KQbPN6J/Attack.png",
    L"https://i.postimg.cc/gLJnm0w8/Dead.png",
    L"https://i.postimg.cc/SXg4qfcH/Attack.png",
    L"https://i.postimg.cc/zLxN5FTs/Dead.png",
    L"https://i.postimg.cc/S25pYQTD/Attacck.png",
    L"https://i.postimg.cc/Ppckvf3S/Dead.png",
};
const int TEX_COUNT = 6;
const int DOWNLOAD_TIMEOUT_MS = 3000;

struct CellRect {
    int x1, y1, x2, y2;
    bool valid;
    CellRect() : x1(0), y1(0), x2(0), y2(0), valid(false) {}
};
struct SheetInfo {
    std::vector<unsigned char> data;
    int w, h;
    bool loaded;
    std::vector<CellRect> cells;
    SheetInfo() : w(0), h(0), loaded(false) {}
};
SheetInfo sheets[6];
bool anyTextureLoaded = false;
std::wstring netStatus = L"";

// ==================== 地图 ====================
const char* MAP_LAYOUT[MAP_H] = {
    "########################",
    "#......................#",
    "#..####........####....#",
    "#..#..............#....#",
    "#..#....####......#....#",
    "#.......#..#...........#",
    "#.......#..#...........#",
    "#.......####...........#",
    "#......................#",
    "#........####..........#",
    "#........#..#..........#",
    "#........#..#..........#",
    "#........####..........#",
    "#......................#",
    "#....####......####....#",
    "#....#............#....#",
    "#....#............#....#",
    "#....####......####....#",
    "#......................#",
    "#..........####........#",
    "#..........#..#........#",
    "#..........#..#........#",
    "#..........####........#",
    "########################"
};
int worldMap[MAP_H][MAP_W];

// ==================== 调色板 ====================
const int CONSOLE_COLORS[16][3] = {
    {0,0,0},       {0,0,128},     {0,128,0},     {0,128,128},
    {128,0,0},     {128,0,128},   {128,128,0},   {192,192,192},
    {128,128,128}, {0,0,255},     {0,255,0},     {0,255,255},
    {255,0,0},     {255,0,255},   {255,255,0},   {255,255,255}
};
int rgbToIndex(int r, int g, int b) {
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < 16; i++) {
        int dr = r - CONSOLE_COLORS[i][0];
        int dg = g - CONSOLE_COLORS[i][1];
        int db = b - CONSOLE_COLORS[i][2];
        int d = dr * dr + dg * dg + db * db;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

// ==================== 像素缓冲 ====================
unsigned char pxBuf[PIXEL_H][PIXEL_W][3];
double zBuffer[PIXEL_W];
inline unsigned char clamp255(int v) {
    if (v < 0) return 0; if (v > 255) return 255;
    return (unsigned char)v;
}
inline void setPixel(int x, int y, int r, int g, int b) {
    if (x < 0 || x >= PIXEL_W || y < 0 || y >= PIXEL_H) return;
    pxBuf[y][x][0] = clamp255(r);
    pxBuf[y][x][1] = clamp255(g);
    pxBuf[y][x][2] = clamp255(b);
}
inline void addPixel(int x, int y, int r, int g, int b) {
    if (x < 0 || x >= PIXEL_W || y < 0 || y >= PIXEL_H) return;
    pxBuf[y][x][0] = clamp255(pxBuf[y][x][0] + r);
    pxBuf[y][x][1] = clamp255(pxBuf[y][x][1] + g);
    pxBuf[y][x][2] = clamp255(pxBuf[y][x][2] + b);
}
inline bool isBackground(const unsigned char* p) {
    if (p[3] < 100) return true;
    if (p[0] > 240 && p[1] > 240 && p[2] > 240) return true;
    return false;
}

// ==================== 自动检测小人 ====================
void autoDetectCells(SheetInfo& sheet) {
    if (!sheet.loaded || sheet.w == 0 || sheet.h == 0) return;
    int W = sheet.w, H = sheet.h;
    sheet.cells.clear();
    std::vector<char> colHasContent(W, 0);
    for (int x = 0; x < W; x++)
        for (int y = 0; y < H; y++) {
            int idx = (y * W + x) * 4;
            if (!isBackground(&sheet.data[idx])) { colHasContent[x] = 1; break; }
        }
    std::vector<std::pair<int, int>> colRanges;
    bool inRange = false; int startX = 0;
    for (int x = 0; x < W; x++) {
        if (colHasContent[x] && !inRange) { inRange = true; startX = x; }
        else if (!colHasContent[x] && inRange) {
            inRange = false;
            if (x - startX >= 8) colRanges.push_back({ startX, x - 1 });
        }
    }
    if (inRange && W - startX >= 8) colRanges.push_back({ startX, W - 1 });
    for (size_t i = 0; i < colRanges.size(); i++) {
        int x1 = colRanges[i].first, x2 = colRanges[i].second;
        int yMin = H, yMax = 0;
        for (int x = x1; x <= x2; x++)
            for (int y = 0; y < H; y++) {
                int idx = (y * W + x) * 4;
                if (!isBackground(&sheet.data[idx])) {
                    if (y < yMin) yMin = y;
                    if (y > yMax) yMax = y;
                }
            }
        if (yMin <= yMax) {
            CellRect cr;
            cr.x1 = x1; cr.y1 = yMin; cr.x2 = x2; cr.y2 = yMax;
            cr.valid = true;
            sheet.cells.push_back(cr);
        }
    }
}

// ==================== 实体 ====================
struct Player {
    double x, y, dirX, dirY, planeX, planeY;
    int hp, maxHp, ammo, score;
    bool alive;
    int hurtFlash;
    int currentWeapon;
    int magAmmo[4];
    int grenades;
    int levelStartHp;
    bool tookDamageLevel;
};
struct Enemy {
    double x, y;
    int hp, maxHp;
    bool alive;
    double hitFlash;
    int attackTimer, spawnAnim, type, strafeTimer, healTimer;
    double strafeDir, opacity;
};
struct Ally {
    double x, y;
    int hp;
    bool alive;
    double hitFlash;
    int attackTimer, targetIdx, muzzleFlash;
    double faceDirX, faceDirY;
};
struct Particle {
    double x, y, vx, vy;
    int life, maxLife, r, g, b;
};
struct LightSource {
    double x, y, radius;
    int r, g, b, life;
};
struct Grenade {
    double x, y, vx, vy;
    int fuse;
    bool alive;
};
struct DeathAnim {
    double x, y;
    int life, maxLife, type;
};
struct MuzzleFlash {
    double x, y, dirX, dirY;
    int life, maxLife, size;
};

Player player;
std::vector<Enemy> enemies;
std::vector<Ally> allies;
std::vector<Particle> particles;
std::vector<LightSource> lights;
std::vector<Grenade> grenades;
std::vector<DeathAnim> deathAnims;
std::vector<MuzzleFlash> muzzleFlashes;

int shootCd = 0, muzzleFlash = 0, reloadTimer = 0;
bool reloading = false;
int hitMarker = 0, spawnTimer = 0, waveCount = 1, killCount = 0;
int hurtFlash = 0, killFlash = 0, totalKills = 0, waveTimer = 0, playTime = 0;
int levelKillsAtStart = 0;
int currentLevel = 0;
const char* SAVE_FILE = "zero_terminal.sav";

// ==================== 存档 ====================
struct SaveData {
    int version, difficulty, playerHp, playerAmmo, playerScore;
    double playerX, playerY, dirX, dirY, planeX, planeY;
    int totalKills, waveCount, playTime, currentLevel;
    int settingsShowRadar, settingsShowCrosshair, settingsShowFps, settingsSoundOn;
    int weaponAmmo[4];
    int endlessMode, endlessWave, missionsCompleted;
};
int missionsCompleted = 0;

void saveGame() {
    SaveData sd;
    sd.version = 3;
    sd.difficulty = (int)currentDiff;
    sd.playerHp = player.hp;
    sd.playerAmmo = player.magAmmo[player.currentWeapon];
    sd.playerScore = player.score;
    sd.playerX = player.x; sd.playerY = player.y;
    sd.dirX = player.dirX; sd.dirY = player.dirY;
    sd.planeX = player.planeX; sd.planeY = player.planeY;
    sd.totalKills = totalKills; sd.waveCount = waveCount;
    sd.playTime = playTime; sd.currentLevel = currentLevel;
    sd.settingsShowRadar = settings.showRadar;
    sd.settingsShowCrosshair = settings.showCrosshair;
    sd.settingsShowFps = settings.showFps;
    sd.settingsSoundOn = settings.soundOn;
    for (int i = 0; i < 4; i++) sd.weaponAmmo[i] = player.magAmmo[i];
    sd.endlessMode = endlessMode ? 1 : 0;
    sd.endlessWave = endlessWave;
    sd.missionsCompleted = missionsCompleted;
    std::ofstream f(SAVE_FILE, std::ios::binary);
    if (f) f.write((char*)&sd, sizeof(sd));
}
bool loadGame(SaveData& sd) {
    std::ifstream f(SAVE_FILE, std::ios::binary);
    if (!f) return false;
    f.read((char*)&sd, sizeof(sd));
    return f.good();
}
bool hasSave() {
    std::ifstream f(SAVE_FILE, std::ios::binary);
    return f.good();
}

// ==================== 初始化 ====================
void initMap() {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            char c = MAP_LAYOUT[y][x];
            worldMap[y][x] = (c == '#') ? (1 + ((x * 3 + y * 5) % 3)) : 0;
        }
}

// ==================== 粒子/光源 ====================
void spawnHitParticles(double x, double y, int r, int g, int b, int count) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = x; p.y = y;
        double ang = (rand() % 628) / 100.0;
        double spd = 0.01 + (rand() % 40) / 1000.0;
        p.vx = cos(ang) * spd; p.vy = sin(ang) * spd;
        p.maxLife = 12 + rand() % 12; p.life = p.maxLife;
        p.r = r; p.g = g; p.b = b;
        particles.push_back(p);
    }
}
void spawnLight(double x, double y, double radius, int r, int g, int b, int life) {
    LightSource ls;
    ls.x = x; ls.y = y; ls.radius = radius;
    ls.r = r; ls.g = g; ls.b = b; ls.life = life;
    lights.push_back(ls);
}
void spawnDeathAnim(double x, double y, int type) {
    DeathAnim da;
    da.x = x; da.y = y;
    da.maxLife = 30; da.life = da.maxLife; da.type = type;
    deathAnims.push_back(da);
}

// ==================== WinHTTP ====================
std::vector<unsigned char> downloadFromURL(const std::wstring& url) {
    std::vector<unsigned char> result;
    URL_COMPONENTS uc;
    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = { 0 };
    wchar_t path[2048] = { 0 };
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;  uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) return result;
    HINTERNET hSession = WinHttpOpen(L"ZERO_TERMINAL/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return result;
    WinHttpSetTimeouts(hSession, DOWNLOAD_TIMEOUT_MS, DOWNLOAD_TIMEOUT_MS,
        DOWNLOAD_TIMEOUT_MS, DOWNLOAD_TIMEOUT_MS);
    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return result; }
    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return result;
    }
    DWORD statusCode = 0, statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
        WINHTTP_NO_HEADER_INDEX);
    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); return result;
    }
    DWORD bytesAvail = 0;
    do {
        bytesAvail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvail)) break;
        if (bytesAvail == 0) break;
        size_t oldSize = result.size();
        result.resize(oldSize + bytesAvail);
        DWORD downloaded = 0;
        if (!WinHttpReadData(hRequest, &result[oldSize], bytesAvail, &downloaded)) {
            result.resize(oldSize); break;
        }
        result.resize(oldSize + downloaded);
    } while (bytesAvail > 0);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

// ==================== WIC 解码 ====================
bool decodeImageWIC(const std::vector<unsigned char>& data, SheetInfo& out) {
    if (data.empty()) return false;
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool needUninit = SUCCEEDED(hr);
    IWICImagingFactory* pFactory = NULL;
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, (LPVOID*)&pFactory);
    if (FAILED(hr)) { if (needUninit) CoUninitialize(); return false; }
    IWICStream* pStream = NULL;
    hr = pFactory->CreateStream(&pStream);
    if (FAILED(hr)) { pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    hr = pStream->InitializeFromMemory((BYTE*)data.data(), (DWORD)data.size());
    if (FAILED(hr)) { pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr)) { pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { pDecoder->Release(); pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    IWICFormatConverter* pConverter = NULL;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { pFrame->Release(); pDecoder->Release(); pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    UINT width = 0, height = 0;
    pConverter->GetSize(&width, &height);
    if (width == 0 || height == 0) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    out.w = (int)width; out.h = (int)height;
    out.data.resize(width * height * 4);
    hr = pConverter->CopyPixels(NULL, width * 4, (UINT)out.data.size(), out.data.data());
    if (FAILED(hr)) { out.data.clear(); out.w = out.h = 0; pConverter->Release(); pFrame->Release(); pDecoder->Release(); pStream->Release(); pFactory->Release(); if (needUninit) CoUninitialize(); return false; }
    out.loaded = true;
    autoDetectCells(out);
    pConverter->Release(); pFrame->Release(); pDecoder->Release(); pStream->Release(); pFactory->Release();
    if (needUninit) CoUninitialize();
    return true;
}

void loadAllTextures() {
    netStatus = L"\u6B63\u5728\u4E0B\u8F7D\u8D34\u56FE...";
    anyTextureLoaded = false;
    int successCount = 0;
    for (int i = 0; i < TEX_COUNT; i++) {
        std::vector<unsigned char> data = downloadFromURL(TEX_URLS[i]);
        if (data.empty()) continue;
        SheetInfo si;
        if (decodeImageWIC(data, si)) { sheets[i] = std::move(si); successCount++; }
    }
    anyTextureLoaded = (successCount > 0);
    if (anyTextureLoaded) {
        wchar_t buf[128];
        swprintf_s(buf, L"\u8D34\u56FE\u52A0\u8F7D\u6210\u529F (%d/%d)", successCount, TEX_COUNT);
        netStatus = buf;
    }
    else {
        netStatus = L"\u7F51\u7EDC\u4E0D\u53EF\u7528\uFF0C\u4F7F\u7528\u539F\u7248\u6E32\u67D3";
    }
}

// ==================== 生成敌人 ====================
bool spawnEnemy() {
    for (int attempt = 0; attempt < 40; attempt++) {
        int ex = 1 + rand() % (MAP_W - 2);
        int ey = 1 + rand() % (MAP_H - 2);
        if (worldMap[ey][ex] != 0) continue;
        double dx = ex + 0.5 - player.x;
        double dy = ey + 0.5 - player.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 6.0) continue;
        bool overlap = false;
        for (int i = 0; i < (int)enemies.size(); i++) {
            if (!enemies[i].alive) continue;
            double edx = enemies[i].x - (ex + 0.5);
            double edy = enemies[i].y - (ey + 0.5);
            if (edx * edx + edy * edy < 1.0) { overlap = true; break; }
        }
        if (overlap) continue;

        Enemy e;
        e.x = ex + 0.5; e.y = ey + 0.5;
        e.alive = true; e.hitFlash = 0;
        e.attackTimer = 0; e.spawnAnim = 15;
        e.strafeDir = (rand() % 2) ? 1 : -1;
        e.strafeTimer = 30 + rand() % 60;
        e.healTimer = 0; e.opacity = 1.0;

        int hpBonus;
        if (endlessMode) {
            hpBonus = endlessWave / 5;
        }
        else {
            hpBonus = MISSIONS[currentLevel].enemyHpBonus;
        }

        if (waveCount >= 10 && waveCount % 5 == 0 && rand() % 100 < 15) {
            e.type = ET_BOSS; e.hp = 40 + hpBonus * 5;
        }
        else if (waveCount >= 4 && rand() % 100 < 12) {
            e.type = ET_GHOST; e.hp = 4 + hpBonus; e.opacity = 0.3;
        }
        else if (waveCount >= 3 && rand() % 100 < 15) {
            e.type = ET_HEALER; e.hp = 3 + hpBonus;
        }
        else {
            int roll = rand() % 100;
            if (waveCount >= 3 && roll < 20) { e.type = ET_TANK; e.hp = 6 + hpBonus; }
            else if (waveCount >= 2 && roll < 45) { e.type = ET_FAST; e.hp = 2 + hpBonus; }
            else if (waveCount >= 4 && roll < 60) { e.type = ET_SNIPER; e.hp = 2 + hpBonus; }
            else { e.type = ET_NORMAL; e.hp = 3 + hpBonus; }
        }
        e.maxHp = e.hp;
        enemies.push_back(e);
        return true;
    }
    return false;
}

// ==================== 生成队友 ====================
bool spawnAlly() {
    for (int attempt = 0; attempt < 40; attempt++) {
        int radius = 2 + rand() % 3;
        double ang = (rand() % 628) / 100.0;
        int ax = (int)(player.x + cos(ang) * radius);
        int ay = (int)(player.y + sin(ang) * radius);
        if (ax < 1 || ax >= MAP_W - 1 || ay < 1 || ay >= MAP_H - 1) continue;
        if (worldMap[ay][ax] != 0) continue;
        bool overlap = false;
        for (int i = 0; i < (int)allies.size(); i++) {
            if (!allies[i].alive) continue;
            double adx = allies[i].x - (ax + 0.5);
            double ady = allies[i].y - (ay + 0.5);
            if (adx * adx + ady * ady < 0.8) { overlap = true; break; }
        }
        if (overlap) continue;

        Ally a;
        a.x = ax + 0.5; a.y = ay + 0.5;
        a.hp = ALLY_HP; a.alive = true;
        a.hitFlash = 0; a.attackTimer = 0;
        a.targetIdx = -1;
        a.faceDirX = 0; a.faceDirY = -1;
        a.muzzleFlash = 0;
        allies.push_back(a);
        return true;
    }
    return false;
}

// ==================== 重置 ====================
void resetPlayerWeapons() {
    player.currentWeapon = 0;
    for (int i = 0; i < 4; i++) player.magAmmo[i] = WEAPONS[i].magSize;
    player.grenades = 3;
}
void resetGame() {
    player.x = 12.0; player.y = 13.5;
    player.dirX = 0.0; player.dirY = -1.0;
    player.planeX = 0.66; player.planeY = 0.0;
    player.hp = diffCfg->startHp;
    player.maxHp = diffCfg->startHp;
    player.score = 0; player.alive = true; player.hurtFlash = 0;
    player.levelStartHp = player.hp;
    player.tookDamageLevel = false;
    resetPlayerWeapons();
    enemies.clear(); allies.clear();
    particles.clear(); lights.clear(); grenades.clear();
    deathAnims.clear(); muzzleFlashes.clear();
    for (int i = 0; i < 4; i++) spawnEnemy();
    spawnAlly(); spawnAlly();
    shootCd = 0; muzzleFlash = 0;
    reloadTimer = 0; reloading = false;
    hitMarker = 0;
    spawnTimer = diffCfg->spawnInterval;
    waveCount = 1; killCount = 0;
    hurtFlash = 0; killFlash = 0;
    totalKills = 0; waveTimer = 0;
    playTime = 0;
    currentLevel = 0;
    endlessMode = false;
    endlessWave = 0;
    levelKillsAtStart = 0;
    statGrenadeKills = 0;
    statGrenadeMaxKills = 0;
    for (int i = 0; i < 4; i++) weaponUsed[i] = false;
    tookDamageThisLevel = false;
    for (int i = 0; i < PIXEL_W; i++) zBuffer[i] = 1e9;
}

void restoreFromSave(SaveData& sd) {
    currentDiff = (Difficulty)sd.difficulty;
    diffCfg = &DIFFS[currentDiff];
    player.x = sd.playerX; player.y = sd.playerY;
    player.dirX = sd.dirX; player.dirY = sd.dirY;
    player.planeX = sd.planeX; player.planeY = sd.planeY;
    player.hp = sd.playerHp;
    player.maxHp = diffCfg->startHp;
    player.score = sd.playerScore;
    player.alive = true; player.hurtFlash = 0;
    player.currentWeapon = 0;
    for (int i = 0; i < 4; i++)
        player.magAmmo[i] = (i < 4) ? sd.weaponAmmo[i] : WEAPONS[i].magSize;
    player.grenades = 3;
    player.levelStartHp = player.hp;
    player.tookDamageLevel = false;
    totalKills = sd.totalKills;
    waveCount = sd.waveCount;
    playTime = sd.playTime;
    currentLevel = sd.currentLevel;
    if (currentLevel < 0 || currentLevel >= TOTAL_MISSIONS) currentLevel = 0;
    endlessMode = (sd.endlessMode != 0);
    endlessWave = sd.endlessWave;
    missionsCompleted = sd.missionsCompleted;
    settings.showRadar = sd.settingsShowRadar;
    settings.showCrosshair = sd.settingsShowCrosshair;
    settings.showFps = sd.settingsShowFps;
    settings.soundOn = sd.settingsSoundOn;
    soundEnabled = settings.soundOn != 0;
    enemies.clear(); allies.clear();
    particles.clear(); lights.clear(); grenades.clear();
    deathAnims.clear(); muzzleFlashes.clear();
    for (int i = 0; i < 4; i++) spawnEnemy();
    spawnAlly(); spawnAlly();
    shootCd = 0; muzzleFlash = 0;
    reloadTimer = 0; reloading = false;
    hitMarker = 0;
    spawnTimer = diffCfg->spawnInterval;
    killCount = 0;
    hurtFlash = 0; killFlash = 0;
    waveTimer = 0;
    levelKillsAtStart = totalKills;
    tookDamageThisLevel = false;
    for (int i = 0; i < PIXEL_W; i++) zBuffer[i] = 1e9;
}

// ==================== 键盘 ====================
bool keyDown(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
struct KeyEdge {
    bool prev[256];
    bool pressed[256];
    KeyEdge() {
        for (int i = 0; i < 256; i++) prev[i] = false;
        for (int i = 0; i < 256; i++) pressed[i] = false;
    }
    void update() {
        for (int i = 0; i < 256; i++) {
            bool cur = (GetAsyncKeyState(i) & 0x8000) != 0;
            pressed[i] = cur && !prev[i];
            prev[i] = cur;
        }
    }
};
KeyEdge keys;

// ==================== 视线 ====================
bool lineBlocked(double ox, double oy, double tx, double ty) {
    double dx = tx - ox, dy = ty - oy;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.01) return false;
    int steps = (int)(dist * 15) + 1;
    for (int i = 1; i < steps; i++) {
        double t = (double)i / steps;
        int mx = (int)(ox + dx * t);
        int my = (int)(oy + dy * t);
        if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return true;
        if (worldMap[my][mx] > 0) return true;
    }
    return false;
}

// ==================== 武器射击 ====================
void fire() {
    if (reloading || shootCd > 0) return;
    WeaponDef& w = WEAPONS[player.currentWeapon];
    if (player.magAmmo[player.currentWeapon] <= 0) {
        reloading = true; reloadTimer = w.reloadTime;
        playSound(SND_RELOAD); return;
    }
    player.magAmmo[player.currentWeapon]--;
    shootCd = w.shootCd;
    muzzleFlash = 5;
    weaponUsed[player.currentWeapon] = true;
    playSound(w.soundId);
    spawnLight(player.x + player.dirX * 0.5, player.y + player.dirY * 0.5,
        4.0, 255, 200, 80, 3);
    {
        MuzzleFlash mf;
        mf.x = player.x + player.dirX * 0.4;
        mf.y = player.y + player.dirY * 0.4;
        mf.dirX = player.dirX; mf.dirY = player.dirY;
        mf.maxLife = 6; mf.life = mf.maxLife; mf.size = 3;
        muzzleFlashes.push_back(mf);
    }
    for (int pellet = 0; pellet < w.pellets; pellet++) {
        double spreadX = ((rand() % 1000) / 500.0 - 1.0) * w.spread;
        double spreadY = ((rand() % 1000) / 500.0 - 1.0) * w.spread;
        double dirX = player.dirX + player.planeX * spreadX;
        double dirY = player.dirY + player.planeY * spreadY;
        double dlen = sqrt(dirX * dirX + dirY * dirY);
        if (dlen > 0.001) { dirX /= dlen; dirY /= dlen; }
        double bestT = 1e9; int hitIdx = -1;
        for (int i = 0; i < (int)enemies.size(); i++) {
            if (!enemies[i].alive) continue;
            double ex = enemies[i].x - player.x;
            double ey = enemies[i].y - player.y;
            double t = ex * dirX + ey * dirY;
            if (t < 0.15 || t > w.range) continue;
            double perpX = ex - t * dirX;
            double perpY = ey - t * dirY;
            double perpDist = sqrt(perpX * perpX + perpY * perpY);
            double hitRadius = 0.32 + t * 0.018;
            if (perpDist < hitRadius && t < bestT) { bestT = t; hitIdx = i; }
        }
        if (hitIdx >= 0 && lineBlocked(player.x, player.y,
            enemies[hitIdx].x, enemies[hitIdx].y)) hitIdx = -1;
        if (hitIdx >= 0) {
            hitMarker = 6;
            Enemy& e = enemies[hitIdx];
            e.hp -= w.dmgPerPellet;
            e.hitFlash = 0.15; e.opacity = 1.0;
            spawnHitParticles(e.x, e.y, 255, 255, 255, 4);
            playSound(SND_HIT);
            if (e.hp <= 0) {
                e.alive = false;
                spawnDeathAnim(e.x, e.y, e.type);
                int bonus = 100;
                switch (e.type) {
                case ET_TANK: bonus = 250; break;
                case ET_FAST: bonus = 150; break;
                case ET_SNIPER: bonus = 200; break;
                case ET_BOSS: bonus = 1000; break;
                case ET_HEALER: bonus = 250; break;
                case ET_GHOST: bonus = 300; break;
                }
                player.score += bonus;
                killCount++; totalKills++; statTotalKills++;
                killFlash = 4;
                playSound(SND_KILL);
                spawnHitParticles(e.x, e.y, 255, 200, 200, 12);
                if (statTotalKills >= 1) unlockAchievement(0);
                if (statTotalKills >= 100) unlockAchievement(1);
            }
        }
    }
}

// ==================== 手雷 ====================
void throwGrenade() {
    if (player.grenades <= 0) return;
    player.grenades--;
    playSound(SND_GRENADE_THROW);
    Grenade g;
    g.x = player.x + player.dirX * 0.3;
    g.y = player.y + player.dirY * 0.3;
    double speed = 0.25;
    g.vx = player.dirX * speed; g.vy = player.dirY * speed;
    g.fuse = 50; g.alive = true;
    grenades.push_back(g);
}
//第一段结束 第二段开始
// ==================== 更新 ====================
void update() {
    if (player.hurtFlash > 0) player.hurtFlash--;
    if (hurtFlash > 0) hurtFlash--;
    if (killFlash > 0) killFlash--;
    waveTimer++;
    playTime++;

    double moveX = 0, moveY = 0;
    if (keyDown('W')) { moveX += player.dirX; moveY += player.dirY; }
    if (keyDown('S')) { moveX -= player.dirX; moveY -= player.dirY; }
    if (keyDown('A')) { moveX += player.dirY; moveY -= player.dirX; }
    if (keyDown('D')) { moveX -= player.dirY; moveY += player.dirX; }
    double len = sqrt(moveX * moveX + moveY * moveY);
    if (len > 0.001) {
        moveX = moveX / len * MOVE_SPEED;
        moveY = moveY / len * MOVE_SPEED;
        double nx = player.x + moveX, ny = player.y + moveY;
        if (worldMap[(int)player.y][(int)(nx + (moveX > 0 ? PLAYER_RAD : -PLAYER_RAD))] == 0)
            player.x = nx;
        if (worldMap[(int)(ny + (moveY > 0 ? PLAYER_RAD : -PLAYER_RAD))][(int)player.x] == 0)
            player.y = ny;
    }

    double rot = 0;
    if (keyDown(VK_LEFT) || keyDown('Q')) rot -= ROT_SPEED;
    if (keyDown(VK_RIGHT) || keyDown('E')) rot += ROT_SPEED;
    if (rot != 0) {
        double oldDirX = player.dirX;
        player.dirX = player.dirX * cos(rot) - player.dirY * sin(rot);
        player.dirY = oldDirX * sin(rot) + player.dirY * cos(rot);
        double oldPlaneX = player.planeX;
        player.planeX = player.planeX * cos(rot) - player.planeY * sin(rot);
        player.planeY = oldPlaneX * sin(rot) + player.planeY * cos(rot);
    }

    if (keys.pressed['1']) player.currentWeapon = 0;
    if (keys.pressed['2']) player.currentWeapon = 1;
    if (keys.pressed['3']) player.currentWeapon = 2;
    if (keys.pressed['4']) player.currentWeapon = 3;

    if (shootCd > 0) shootCd--;
    if (keyDown(VK_SPACE) && !reloading) fire();
    if (keys.pressed['G'] && !reloading) throwGrenade();

    if (keyDown('R') && !reloading && reloadTimer == 0) {
        WeaponDef& w = WEAPONS[player.currentWeapon];
        if (player.magAmmo[player.currentWeapon] < w.magSize) {
            reloading = true; reloadTimer = w.reloadTime;
            playSound(SND_RELOAD);
        }
    }
    if (reloading) {
        reloadTimer--;
        if (reloadTimer <= 0) {
            player.magAmmo[player.currentWeapon] = WEAPONS[player.currentWeapon].magSize;
            reloading = false;
        }
    }
    if (muzzleFlash > 0) muzzleFlash--;
    if (hitMarker > 0) hitMarker--;

    // 手雷更新
    for (int i = (int)grenades.size() - 1; i >= 0; i--) {
        Grenade& g = grenades[i];
        g.x += g.vx; g.y += g.vy;
        int gx = (int)g.x, gy = (int)g.y;
        if (gx < 0 || gx >= MAP_W || gy < 0 || gy >= MAP_H ||
            worldMap[gy][gx] > 0) {
            g.vx = 0; g.vy = 0;
        }
        g.fuse--;
        if (g.fuse <= 0) {
            playSound(SND_GRENADE_BOOM);
            spawnLight(g.x, g.y, 6.0, 255, 180, 50, 6);
            int killInBoom = 0;
            for (int j = 0; j < (int)enemies.size(); j++) {
                if (!enemies[j].alive) continue;
                double dx = enemies[j].x - g.x;
                double dy = enemies[j].y - g.y;
                double d = sqrt(dx * dx + dy * dy);
                if (d < 2.5) {
                    int dmg = (int)(80 * (1.0 - d / 2.5));
                    if (dmg < 20) dmg = 20;
                    enemies[j].hp -= dmg;
                    enemies[j].hitFlash = 0.2;
                    enemies[j].opacity = 1.0;
                    if (enemies[j].hp <= 0) {
                        enemies[j].alive = false;
                        spawnDeathAnim(enemies[j].x, enemies[j].y, enemies[j].type);
                        player.score += 150;
                        killCount++; totalKills++; statTotalKills++;
                        statGrenadeKills++;
                        killInBoom++;
                        spawnHitParticles(enemies[j].x, enemies[j].y, 255, 180, 80, 12);
                    }
                }
            }
            if (killInBoom > statGrenadeMaxKills) statGrenadeMaxKills = killInBoom;
            if (statGrenadeKills >= 10) unlockAchievement(2);
            if (statGrenadeMaxKills >= 3) unlockAchievement(5);
            spawnHitParticles(g.x, g.y, 255, 220, 100, 40);
            grenades.erase(grenades.begin() + i);
        }
    }

    // 队友 AI
    for (int i = 0; i < (int)allies.size(); i++) {
        Ally& a = allies[i];
        if (!a.alive) continue;
        if (a.hitFlash > 0) a.hitFlash -= 1.0 / 30.0;
        if (a.muzzleFlash > 0) a.muzzleFlash--;
        if (a.attackTimer > 0) a.attackTimer--;

        int closest = -1;
        double closestDist = ALLY_SIGHT;
        for (int j = 0; j < (int)enemies.size(); j++) {
            if (!enemies[j].alive || enemies[j].spawnAnim > 0) continue;
            double dx = enemies[j].x - a.x, dy = enemies[j].y - a.y;
            double d = sqrt(dx * dx + dy * dy);
            if (d < closestDist && !lineBlocked(a.x, a.y, enemies[j].x, enemies[j].y)) {
                closestDist = d; closest = j;
            }
        }
        a.targetIdx = closest;

        if (closest >= 0) {
            Enemy& e = enemies[closest];
            double dx = e.x - a.x, dy = e.y - a.y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.01) { a.faceDirX = dx / dist; a.faceDirY = dy / dist; }
            if (dist > 4.5) {
                double nx = a.x + a.faceDirX * ALLY_SPEED;
                double ny = a.y + a.faceDirY * ALLY_SPEED;
                if (worldMap[(int)a.y][(int)nx] == 0) a.x = nx;
                if (worldMap[(int)ny][(int)a.x] == 0) a.y = ny;
            }
            else if (dist < 2.0) {
                double nx = a.x - a.faceDirX * ALLY_SPEED * 0.8;
                double ny = a.y - a.faceDirY * ALLY_SPEED * 0.8;
                if (worldMap[(int)a.y][(int)nx] == 0) a.x = nx;
                if (worldMap[(int)ny][(int)a.x] == 0) a.y = ny;
            }
            if (a.attackTimer <= 0 && dist < 6.5) {
                a.attackTimer = ALLY_ATTACK_CD;
                a.muzzleFlash = 4;
                spawnLight(a.x + a.faceDirX * 0.4, a.y + a.faceDirY * 0.4,
                    2.5, 100, 180, 255, 2);
                if (rand() % 100 < 55) {
                    e.hp--; e.hitFlash = 0.15; e.opacity = 1.0;
                    spawnHitParticles(e.x, e.y, 255, 255, 255, 4);
                    if (e.hp <= 0) {
                        e.alive = false;
                        spawnDeathAnim(e.x, e.y, e.type);
                        spawnHitParticles(e.x, e.y, 255, 200, 200, 12);
                        statTotalKills++;
                        player.score += 50;
                        killCount++;
                    }
                }
            }
        }
        else {
            double dx = player.x - a.x, dy = player.y - a.y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > ALLY_FOLLOW_DIST) {
                double nx = dx / dist, ny = dy / dist;
                a.faceDirX = nx; a.faceDirY = ny;
                double mx = a.x + nx * ALLY_SPEED * 0.7;
                double my = a.y + ny * ALLY_SPEED * 0.7;
                if (worldMap[(int)a.y][(int)mx] == 0) a.x = mx;
                if (worldMap[(int)my][(int)a.x] == 0) a.y = my;
            }
        }
    }

    // 敌人 AI
    for (int i = 0; i < (int)enemies.size(); i++) {
        Enemy& e = enemies[i];
        if (!e.alive) continue;
        if (e.hitFlash > 0) e.hitFlash -= 1.0 / 30.0;
        if (e.spawnAnim > 0) e.spawnAnim--;
        if (e.attackTimer > 0) e.attackTimer--;
        if (e.strafeTimer > 0) e.strafeTimer--;
        else { e.strafeDir = -e.strafeDir; e.strafeTimer = 30 + rand() % 60; }

        if (e.type == ET_GHOST) {
            if (e.hitFlash <= 0) {
                e.opacity -= 0.005;
                if (e.opacity < 0.15) e.opacity = 0.15;
            }
            else e.opacity = 1.0;
        }

        if (e.type == ET_HEALER) {
            e.healTimer++;
            if (e.healTimer >= 60) {
                e.healTimer = 0;
                for (int j = 0; j < (int)enemies.size(); j++) {
                    if (j == i || !enemies[j].alive) continue;
                    double dx = enemies[j].x - e.x, dy = enemies[j].y - e.y;
                    double d = sqrt(dx * dx + dy * dy);
                    if (d < 3.0 && enemies[j].hp < enemies[j].maxHp) {
                        enemies[j].hp += 1;
                        if (enemies[j].hp > enemies[j].maxHp)
                            enemies[j].hp = enemies[j].maxHp;
                        spawnLight(enemies[j].x, enemies[j].y, 1.5, 100, 255, 100, 2);
                    }
                }
            }
        }

        if (e.spawnAnim > 0) continue;

        double bestDist = 1e9;
        int targetType = 0, targetIdx = -1;
        double targetX = player.x, targetY = player.y;

        double dpx = player.x - e.x, dpy = player.y - e.y;
        double pd = sqrt(dpx * dpx + dpy * dpy);
        if (pd < bestDist) { bestDist = pd; targetType = 0; targetIdx = -1; }

        for (int j = 0; j < (int)allies.size(); j++) {
            if (!allies[j].alive) continue;
            double adx = allies[j].x - e.x, ady = allies[j].y - e.y;
            double ad = sqrt(adx * adx + ady * ady);
            if (ad < bestDist) {
                bestDist = ad; targetType = 1; targetIdx = j;
                targetX = allies[j].x; targetY = allies[j].y;
            }
        }

        double dx = targetX - e.x, dy = targetY - e.y;
        double dist = sqrt(dx * dx + dy * dy);

        double atkRange = 0.75, speed = 0.022;
        int dmg = diffCfg->enemyDmg;
        if (e.type == ET_FAST) { speed = 0.04; dmg = diffCfg->enemyDmg - 3; }
        else if (e.type == ET_TANK) { speed = 0.013; atkRange = 0.9; dmg = diffCfg->enemyDmg + 7; }
        else if (e.type == ET_SNIPER) { speed = 0.015; atkRange = 6.0; dmg = diffCfg->enemyDmg - 2; }
        else if (e.type == ET_BOSS) { speed = 0.018; atkRange = 1.2; dmg = diffCfg->enemyDmg + 10; }
        else if (e.type == ET_HEALER) { speed = 0.02; dmg = diffCfg->enemyDmg - 5; }
        else if (e.type == ET_GHOST) { speed = 0.05; dmg = diffCfg->enemyDmg; }

        bool shouldMove = true;
        double moveDirX = 0, moveDirY = 0;
        if (e.type == ET_SNIPER) {
            if (dist > 7.0) { moveDirX = dx / dist; moveDirY = dy / dist; }
            else if (dist < 4.0) { moveDirX = -dx / dist; moveDirY = -dy / dist; }
            else shouldMove = false;
        }
        else {
            if (dist > atkRange) { moveDirX = dx / dist; moveDirY = dy / dist; }
            else shouldMove = false;
        }

        if (waveCount >= 2 && e.type != ET_TANK && e.type != ET_BOSS && dist < 5.0) {
            double perpX = -dy / dist, perpY = dx / dist;
            moveDirX += perpX * e.strafeDir * 0.6;
            moveDirY += perpY * e.strafeDir * 0.6;
            double mlen = sqrt(moveDirX * moveDirX + moveDirY * moveDirY);
            if (mlen > 0.01) { moveDirX /= mlen; moveDirY /= mlen; }
        }

        if (shouldMove && (moveDirX != 0 || moveDirY != 0)) {
            double nx = e.x + moveDirX * speed;
            double ny = e.y + moveDirY * speed;
            if (worldMap[(int)e.y][(int)nx] == 0) e.x = nx;
            if (worldMap[(int)ny][(int)e.x] == 0) e.y = ny;
        }

        if (dist <= atkRange && e.attackTimer <= 0) {
            if (e.type == ET_SNIPER && lineBlocked(e.x, e.y, targetX, targetY)) {
                // skip
            }
            else {
                e.attackTimer = (e.type == ET_FAST) ? 18 :
                    (e.type == ET_TANK) ? 35 :
                    (e.type == ET_SNIPER) ? 55 :
                    (e.type == ET_BOSS) ? 25 :
                    (e.type == ET_HEALER) ? 45 :
                    (e.type == ET_GHOST) ? 20 : diffCfg->attackCd;
                if (targetType == 0) {
                    player.hp -= dmg;
                    player.hurtFlash = 8; hurtFlash = 5;
                    player.tookDamageLevel = true;
                    tookDamageThisLevel = true;
                    playSound(SND_HURT);
                    if (player.hp <= 0) {
                        player.hp = 0; player.alive = false;
                        gameState = GS_GAME_OVER;
                        playSound(SND_GAME_OVER);
                        if (endlessMode) {
                            submitScore(player.score, totalKills, (int)currentDiff, endlessWave);
                        }
                        else {
                            submitScore(player.score, totalKills, (int)currentDiff, currentLevel);
                        }
                        saveGame();
                    }
                }
                else if (targetIdx >= 0 && targetIdx < (int)allies.size()) {
                    allies[targetIdx].hp--;
                    allies[targetIdx].hitFlash = 0.2;
                    if (allies[targetIdx].hp <= 0) {
                        allies[targetIdx].alive = false;
                        spawnHitParticles(allies[targetIdx].x, allies[targetIdx].y,
                            100, 180, 255, 14);
                    }
                }
            }
        }
    }

    for (int i = (int)enemies.size() - 1; i >= 0; i--)
        if (!enemies[i].alive) enemies.erase(enemies.begin() + i);
    for (int i = (int)allies.size() - 1; i >= 0; i--)
        if (!allies[i].alive) allies.erase(allies.begin() + i);

    // 波次推进 & 刷新
    if (--spawnTimer <= 0) {
        int maxEnemyCount;
        if (endlessMode) {
            maxEnemyCount = 20 + endlessWave;
        }
        else {
            maxEnemyCount = MISSIONS[currentLevel].maxEnemies;
        }
        if ((int)enemies.size() < maxEnemyCount) {
            if (spawnEnemy()) {
                int oldWave = waveCount;
                waveCount = 1 + totalKills / 4;

                if (endlessMode) {
                    int newWave = totalKills / 4;
                    if (newWave > endlessWave) {
                        endlessWave = newWave;
                        if (endlessWave % 10 == 0) {
                            playSound(SND_LEVEL_UP);
                        }
                    }
                }
                else {
                    // 普通模式：达到敌人上限且场上清空就过关
                    if (waveCount >= MISSIONS[currentLevel].maxEnemies / 2 &&
                        (int)enemies.size() == 0) {
                        if (!tookDamageThisLevel) unlockAchievement(3);
                        if (currentLevel < TOTAL_MISSIONS - 1) {
                            gameState = GS_LEVEL_COMPLETE;
                            playSound(SND_LEVEL_UP);
                            levelKillsAtStart = totalKills;
                        }
                        else {
                            gameState = GS_LEVEL_COMPLETE;
                            playSound(SND_LEVEL_UP);
                        }
                    }
                }
            }
        }
        int interval;
        if (endlessMode) {
            interval = 40 - endlessWave;
            if (interval < 15) interval = 15;
        }
        else {
            interval = MISSIONS[currentLevel].spawnInterval;
        }
        spawnTimer = interval;

        if ((int)allies.size() < MAX_ALLY && totalKills > 0 && totalKills % 4 == 0)
            if (rand() % 3 == 0) spawnAlly();
    }

    if (weaponUsed[0] && weaponUsed[1] && weaponUsed[2] && weaponUsed[3])
        unlockAchievement(4);

    for (int i = (int)particles.size() - 1; i >= 0; i--) {
        Particle& p = particles[i];
        p.x += p.vx; p.y += p.vy;
        p.vx *= 0.9; p.vy *= 0.9;
        p.life--;
        if (p.life <= 0) particles.erase(particles.begin() + i);
    }
    for (int i = (int)lights.size() - 1; i >= 0; i--) {
        lights[i].life--;
        if (lights[i].life <= 0) lights.erase(lights.begin() + i);
    }
    for (int i = (int)deathAnims.size() - 1; i >= 0; i--) {
        deathAnims[i].life--;
        if (deathAnims[i].life <= 0) deathAnims.erase(deathAnims.begin() + i);
    }
    for (int i = (int)muzzleFlashes.size() - 1; i >= 0; i--) {
        muzzleFlashes[i].life--;
        if (muzzleFlashes[i].life <= 0) muzzleFlashes.erase(muzzleFlashes.begin() + i);
    }
}

// ==================== 光照 ====================
void computeLight(double wx, double wy, int& lr, int& lg, int& lb) {
    lr = 0; lg = 0; lb = 0;
    for (int i = 0; i < (int)lights.size(); i++) {
        LightSource& ls = lights[i];
        double dx = wx - ls.x, dy = wy - ls.y;
        double d2 = dx * dx + dy * dy;
        if (d2 > ls.radius * ls.radius) continue;
        double att = 1.0 - sqrt(d2) / ls.radius;
        double lifeAtt = ls.life / 3.0;
        if (lifeAtt > 1.0) lifeAtt = 1.0;
        att *= lifeAtt;
        if (att < 0) att = 0;
        lr += (int)(ls.r * att);
        lg += (int)(ls.g * att);
        lb += (int)(ls.b * att);
    }
}

// ==================== 背景 ====================
void renderBackground() {
    double angle = atan2(player.dirY, player.dirX);
    int skyOffset = (int)(angle * 30) % PIXEL_W;
    for (int y = 0; y < PIXEL_H / 2; y++) {
        double t = (double)y / (PIXEL_H / 2);
        int base = 12 + (int)(t * 40);
        for (int x = 0; x < PIXEL_W; x++) {
            int xx = (x + skyOffset + PIXEL_W) % PIXEL_W;
            int noise = ((xx * 7 + y * 13) % 7) - 3;
            int cloud = ((xx * 3 - y * 5) % 23);
            int cloudBoost = (cloud < 3) ? 15 : 0;
            setPixel(x, y, base / 3 + noise + cloudBoost / 4,
                base / 3 + noise + cloudBoost / 3,
                base + noise + cloudBoost);
        }
    }
    for (int y = PIXEL_H / 2; y < PIXEL_H; y++) {
        double t = (double)(y - PIXEL_H / 2) / (PIXEL_H / 2);
        int base = 70 - (int)(t * 45);
        int floorOffset = (int)(angle * 40);
        for (int x = 0; x < PIXEL_W; x++) {
            int xx = (x + floorOffset + PIXEL_W * 2) % PIXEL_W;
            int bx = xx / 8, by = y / 4;
            int brickNoise = ((bx * 31 + by * 17) % 7) - 3;
            setPixel(x, y, base + brickNoise + 18,
                base / 2 + brickNoise + 8,
                base / 3 + brickNoise);
        }
    }
}

// ==================== 墙壁 ====================
void renderWalls() {
    for (int x = 0; x < PIXEL_W; x++) {
        double cameraX = 2.0 * x / PIXEL_W - 1.0;
        double rayDirX = player.dirX + player.planeX * cameraX;
        double rayDirY = player.dirY + player.planeY * cameraX;
        int mapX = (int)player.x, mapY = (int)player.y;
        double deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0 / rayDirY);
        double sideDistX, sideDistY;
        int stepX, stepY, side = 0;
        if (rayDirX < 0) { stepX = -1; sideDistX = (player.x - mapX) * deltaDistX; }
        else { stepX = 1; sideDistX = (mapX + 1.0 - player.x) * deltaDistX; }
        if (rayDirY < 0) { stepY = -1; sideDistY = (player.y - mapY) * deltaDistY; }
        else { stepY = 1; sideDistY = (mapY + 1.0 - player.y) * deltaDistY; }

        int hit = 0;
        while (!hit) {
            if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
            else { sideDistY += deltaDistY; mapY += stepY; side = 1; }
            if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H) break;
            if (worldMap[mapY][mapX] > 0) hit = 1;
        }

        double perpDist = (side == 0) ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
        if (perpDist < 0.01) perpDist = 0.01;
        zBuffer[x] = perpDist;

        int lineH = (int)(PIXEL_H / perpDist);
        int dStart = -lineH / 2 + PIXEL_H / 2;
        int dEnd = lineH / 2 + PIXEL_H / 2;
        if (dStart < 0) dStart = 0;
        if (dEnd >= PIXEL_H) dEnd = PIXEL_H - 1;

        double wallX = (side == 0) ? (player.y + perpDist * rayDirY)
            : (player.x + perpDist * rayDirX);
        wallX -= floor(wallX);

        int wType = 1;
        if (mapX >= 0 && mapX < MAP_W && mapY >= 0 && mapY < MAP_H)
            wType = worldMap[mapY][mapX];

        int baseR, baseG, baseB;
        switch (currentLevel % 3) {
        case 0:
            switch (wType) {
            case 1: baseR = 165; baseG = 115; baseB = 75;  break;
            case 2: baseR = 130; baseG = 130; baseB = 145; break;
            case 3: baseR = 100; baseG = 85;  baseB = 60;  break;
            default: baseR = 130; baseG = 95;  baseB = 75;
            }
            break;
        case 1:
            switch (wType) {
            case 1: baseR = 100; baseG = 140; baseB = 100; break;
            case 2: baseR = 90;  baseG = 120; baseB = 110; break;
            case 3: baseR = 70;  baseG = 100; baseB = 80;  break;
            default: baseR = 90; baseG = 120; baseB = 100;
            }
            break;
        default:
            switch (wType) {
            case 1: baseR = 150; baseG = 80;  baseB = 90;  break;
            case 2: baseR = 130; baseG = 70;  baseB = 80;  break;
            case 3: baseR = 100; baseG = 50;  baseB = 60;  break;
            default: baseR = 130; baseG = 70; baseB = 80;
            }
            break;
        }
        if (side == 1) { baseR = baseR * 68 / 100; baseG = baseG * 68 / 100; baseB = baseB * 68 / 100; }

        double fog = 1.0 / (1.0 + perpDist * 0.14);
        if (fog < 0.22) fog = 0.22;

        double wallWorldX = player.x + rayDirX * perpDist;
        double wallWorldY = player.y + rayDirY * perpDist;
        int lightR, lightG, lightB;
        computeLight(wallWorldX, wallWorldY, lightR, lightG, lightB);

        for (int y = dStart; y <= dEnd; y++) {
            double wallY = (double)(y - PIXEL_H / 2 + lineH / 2) / lineH;
            if (wallY < 0) wallY = 0;
            if (wallY > 1) wallY = 1;

            int texX = (int)(wallX * 8);
            int texY = (int)(wallY * 16);
            int texture = ((texX + texY) % 2 == 0) ? 1 : 0;
            int r = baseR, g = baseG, b = baseB;
            if (texture == 0) { r = r * 88 / 100; g = g * 88 / 100; b = b * 88 / 100; }
            if (texY % 4 == 0) { r = r * 55 / 100; g = g * 55 / 100; b = b * 55 / 100; }
            if (texY % 4 == 1) { r = r * 115 / 100; g = g * 115 / 100; b = b * 115 / 100; }
            r = (int)(r * fog); g = (int)(g * fog); b = (int)(b * fog);
            r += lightR; g += lightG; b += lightB;
            setPixel(x, y, r, g, b);
        }
    }
}

// ==================== 精灵渲染 ====================
void renderSprite(double sx, double sy, int sheetIndex, int cellIdx,
    int tintR, int tintG, int tintB,
    double hitFlash, int spawnAnim, int hp, int maxHp, int type,
    double opacity) {
    double spx = sx - player.x, spy = sy - player.y;
    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
    double transX = invDet * (player.dirY * spx - player.dirX * spy);
    double transY = invDet * (-player.planeY * spx + player.planeX * spy);
    if (transY <= 0.15) return;

    int spriteScreenX = (int)((PIXEL_W / 2.0) * (1.0 + transX / transY));
    double scale = 1.0;
    if (spawnAnim > 0) {
        scale = 1.0 - (double)spawnAnim / 15.0;
        if (scale < 0.1) scale = 0.1;
    }
    bool useTex = false;
    const CellRect* cell = nullptr;
    SheetInfo* sheet = nullptr;
    if (sheetIndex >= 0 && sheetIndex < 6) {
        if (sheets[sheetIndex].loaded &&
            cellIdx >= 0 && cellIdx < (int)sheets[sheetIndex].cells.size()) {
            cell = &sheets[sheetIndex].cells[cellIdx];
            sheet = &sheets[sheetIndex];
            if (cell->valid) useTex = true;
        }
    }
    double spriteH, spriteW;
    if (useTex) {
        int cellW = cell->x2 - cell->x1 + 1;
        int cellH = cell->y2 - cell->y1 + 1;
        spriteH = 0.85 * PIXEL_H / transY * scale;
        double aspect = (double)cellW / (double)cellH;
        spriteW = spriteH * aspect;
    }
    else {
        spriteH = 0.85 * PIXEL_H / transY * scale;
        spriteW = spriteH * 0.5;
    }
    int drawStartY = (int)(PIXEL_H / 2.0 - spriteH * 0.3);
    int drawEndY = (int)(PIXEL_H / 2.0 + spriteH * 0.7);
    int drawStartX = (int)(spriteScreenX - spriteW / 2);
    int drawEndX = (int)(spriteScreenX + spriteW / 2);
    if (drawStartY < 0) drawStartY = 0;
    if (drawEndY >= PIXEL_H) drawEndY = PIXEL_H - 1;
    if (drawStartX < 0) drawStartX = 0;
    if (drawEndX >= PIXEL_W) drawEndX = PIXEL_W - 1;

    int er = tintR, eg = tintG, eb = tintB;
    if (hitFlash > 0) { er = 255; eg = 255; eb = 255; }
    else if (spawnAnim > 0) {
        int flash = spawnAnim % 4;
        if (type == 0) {
            if (flash < 2) { er = 50; eg = 255; eb = 100; }
            else { er = 200; eg = 200; eb = 200; }
        }
        else {
            if (flash < 2) { er = 100; eg = 200; eb = 255; }
            else { er = 60; eg = 120; eb = 220; }
        }
    }
    double fogE = 1.0 / (1.0 + transY * 0.12);
    if (fogE < 0.55) fogE = 0.55;
    int lightR = 0, lightG = 0, lightB = 0;
    if (type == 0) computeLight(sx, sy, lightR, lightG, lightB);
    for (int x = drawStartX; x <= drawEndX; x++) {
        if (transY >= zBuffer[x]) continue;
        for (int y = drawStartY; y <= drawEndY; y++) {
            double relY = (double)(y - drawStartY) / (drawEndY - drawStartY + 1);
            double relX = (double)(x - drawStartX) / (drawEndX - drawStartX + 1);
            int hr = 0, hg = 0, hb = 0;
            bool draw = false;
            if (useTex) {
                int cellW = cell->x2 - cell->x1 + 1;
                int cellH = cell->y2 - cell->y1 + 1;
                int px = cell->x1 + (int)(relX * cellW);
                int py = cell->y1 + (int)(relY * cellH);
                if (px < 0) px = 0; if (px >= sheet->w) px = sheet->w - 1;
                if (py < 0) py = 0; if (py >= sheet->h) py = sheet->h - 1;
                int idx = (py * sheet->w + px) * 4;
                if (!isBackground(&sheet->data[idx])) {
                    int tr = sheet->data[idx];
                    int tg = sheet->data[idx + 1];
                    int tb = sheet->data[idx + 2];
                    int lum = (tr + tg + tb) / 3;
                    double shade = 0.5 + (lum / 255.0) * 0.8;
                    if (shade > 1.4) shade = 1.4;
                    hr = (int)(er * shade);
                    hg = (int)(eg * shade);
                    hb = (int)(eb * shade);
                    draw = true;
                }
            }
            else {
                bool body = false;
                if (relY < 0.20) {
                    double hdx = (relX - 0.5) * 2.3;
                    double hdy = (relY - 0.10) * 2.3;
                    if (hdx * hdx + hdy * hdy < 0.09) body = true;
                }
                else if (relY < 0.45) {
                    if (relX > 0.15 && relX < 0.85) body = true;
                }
                else if (relY < 0.80) {
                    if (relX > 0.20 && relX < 0.80) body = true;
                }
                else {
                    if ((relX > 0.25 && relX < 0.45) || (relX > 0.55 && relX < 0.75)) body = true;
                }
                if (body) {
                    hr = er; hg = eg; hb = eb;
                    draw = true;
                }
            }
            if (draw) {
                hr = (int)(hr * fogE); hg = (int)(hg * fogE); hb = (int)(hb * fogE);
                if (hitFlash > 0) { hr = 255; hg = 255; hb = 255; }
                hr += lightR; hg += lightG; hb += lightB;
                if (opacity < 1.0) {
                    int br = pxBuf[y][x][0], bg = pxBuf[y][x][1], bb2 = pxBuf[y][x][2];
                    hr = (int)(hr * opacity + br * (1.0 - opacity));
                    hg = (int)(hg * opacity + bg * (1.0 - opacity));
                    hb = (int)(hb * opacity + bb2 * (1.0 - opacity));
                }
                setPixel(x, y, hr, hg, hb);
            }
        }
    }
    if (hp < maxHp && spawnAnim <= 0 && transY > 0.5) {
        int barW = (int)(spriteW * 0.8);
        if (barW < 4) barW = 4;
        int barX = spriteScreenX - barW / 2;
        int barY = drawStartY - 3;
        if (barY < 0) barY = 0;
        int fillW = barW * hp / maxHp;
        for (int x = 0; x < barW; x++) {
            if (barX + x < 0 || barX + x >= PIXEL_W) continue;
            if (x < fillW) setPixel(barX + x, barY, 255, 60, 60);
            else setPixel(barX + x, barY, 40, 20, 20);
        }
    }
}

// ==================== 渲染实体 ====================
void renderEntities() {
    struct Item { double dist; int type; int idx; };
    std::vector<Item> items;
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (!enemies[i].alive) continue;
        double dx = enemies[i].x - player.x, dy = enemies[i].y - player.y;
        Item it; it.dist = dx * dx + dy * dy; it.type = 0; it.idx = i;
        items.push_back(it);
    }
    for (int i = 0; i < (int)allies.size(); i++) {
        if (!allies[i].alive) continue;
        double dx = allies[i].x - player.x, dy = allies[i].y - player.y;
        Item it; it.dist = dx * dx + dy * dy; it.type = 1; it.idx = i;
        items.push_back(it);
    }
    std::sort(items.begin(), items.end(),
        [](const Item& a, const Item& b) { return a.dist > b.dist; });

    for (int k = 0; k < (int)items.size(); k++) {
        Item& it = items[k];
        if (it.type == 0) {
            Enemy& e = enemies[it.idx];
            int sheetIdx = 0, cellIdx = 0;
            int tR = 255, tG = 255, tB = 255;
            double opacity = e.opacity;
            switch (e.type) {
            case ET_NORMAL: sheetIdx = 0; cellIdx = 0; tR = 240; tG = 240; tB = 240; break;
            case ET_FAST:   sheetIdx = 2; cellIdx = 0; tR = 255; tG = 220; tB = 180; break;
            case ET_TANK:   sheetIdx = 4; cellIdx = 0; tR = 200; tG = 200; tB = 200; break;
            case ET_SNIPER: sheetIdx = 0; cellIdx = 1; tR = 180; tG = 255; tB = 180; break;
            case ET_BOSS:   sheetIdx = 4; cellIdx = 0; tR = 255; tG = 100; tB = 100; break;
            case ET_HEALER: sheetIdx = 2; cellIdx = 0; tR = 100; tG = 255; tB = 140; break;
            case ET_GHOST:  sheetIdx = 0; cellIdx = 0; tR = 200; tG = 200; tB = 255; break;
            }
            renderSprite(e.x, e.y, sheetIdx, cellIdx,
                tR, tG, tB, e.hitFlash, e.spawnAnim,
                e.hp, e.maxHp, 0, opacity);
        }
        else {
            Ally& a = allies[it.idx];
            renderSprite(a.x, a.y, 2, 0, 80, 180, 255,
                a.hitFlash, 0, a.hp, ALLY_HP, 1, 1.0);
        }
    }
}

// ==================== 死亡动画渲染 ====================
void renderDeathAnims() {
    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
    for (int i = 0; i < (int)deathAnims.size(); i++) {
        DeathAnim& da = deathAnims[i];
        double spx = da.x - player.x, spy = da.y - player.y;
        double transX = invDet * (player.dirY * spx - player.dirX * spy);
        double transY = invDet * (-player.planeY * spx + player.planeX * spy);
        if (transY <= 0.15) continue;
        int screenX = (int)((PIXEL_W / 2.0) * (1.0 + transX / transY));
        int screenY = PIXEL_H / 2;
        if (screenX < 0 || screenX >= PIXEL_W) continue;
        if (transY >= zBuffer[screenX]) continue;
        double progress = 1.0 - (double)da.life / da.maxLife;
        int size = (int)(6.0 / transY);
        if (size < 2) size = 2;
        if (size > 6) size = 6;
        int r = 200, g = 60, b = 60;
        switch (da.type) {
        case ET_NORMAL: r = 200; g = 200; b = 200; break;
        case ET_FAST:   r = 255; g = 220; b = 180; break;
        case ET_TANK:   r = 200; g = 200; b = 200; break;
        case ET_SNIPER: r = 180; g = 255; b = 180; break;
        case ET_BOSS:   r = 255; g = 100; b = 100; break;
        case ET_HEALER: r = 100; g = 255; b = 140; break;
        case ET_GHOST:  r = 200; g = 200; b = 255; break;
        }
        double darkness = 1.0 - progress * 0.7;
        r = (int)(r * darkness);
        g = (int)(g * darkness);
        b = (int)(b * darkness);
        int h = size, w = size;
        if (progress > 0.5) { h = size / 2; w = size; }
        int offsetY = (int)(progress * size * 0.5);
        for (int dy = -h; dy <= h; dy++)
            for (int dx = -w; dx <= w; dx++) {
                if (dx * dx + dy * dy <= w * w) {
                    addPixel(screenX + dx, screenY + dy + offsetY, r, g, b);
                }
            }
    }
}

// ==================== 枪口火光渲染 ====================
void renderMuzzleFlashes() {
    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
    for (int i = 0; i < (int)muzzleFlashes.size(); i++) {
        MuzzleFlash& mf = muzzleFlashes[i];
        double spx = mf.x - player.x, spy = mf.y - player.y;
        double transX = invDet * (player.dirY * spx - player.dirX * spy);
        double transY = invDet * (-player.planeY * spx + player.planeX * spy);
        if (transY <= 0.15) continue;
        int screenX = (int)((PIXEL_W / 2.0) * (1.0 + transX / transY));
        int screenY = PIXEL_H / 2;
        if (screenX < 0 || screenX >= PIXEL_W) continue;
        double intensity = (double)mf.life / mf.maxLife;
        int size = mf.size + (int)(intensity * 3);
        for (int layer = 0; layer < 3; layer++) {
            int layerSize = size - layer;
            if (layerSize < 1) continue;
            int brightness = (int)(255 * intensity * (1.0 - layer * 0.3));
            for (int dy = -layerSize; dy <= layerSize; dy++)
                for (int dx = -layerSize; dx <= layerSize; dx++) {
                    int d = abs(dx) + abs(dy);
                    if (d <= layerSize) {
                        int fade = (int)(brightness * (1.0 - (double)d / layerSize));
                        addPixel(screenX + dx, screenY + dy, fade, fade * 8 / 10, fade * 3 / 10);
                    }
                }
        }
    }
}

// ==================== 手雷渲染 ====================
void renderGrenades() {
    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
    for (int i = 0; i < (int)grenades.size(); i++) {
        Grenade& g = grenades[i];
        double spx = g.x - player.x, spy = g.y - player.y;
        double transX = invDet * (player.dirY * spx - player.dirX * spy);
        double transY = invDet * (-player.planeY * spx + player.planeX * spy);
        if (transY <= 0.15) continue;
        int screenX = (int)((PIXEL_W / 2.0) * (1.0 + transX / transY));
        int screenY = PIXEL_H / 2;
        if (screenX < 0 || screenX >= PIXEL_W) continue;
        if (transY >= zBuffer[screenX]) continue;
        int r = 60, g2 = 200, b = 60;
        if (g.fuse < 15 && (g.fuse / 3) % 2) { r = 255; g2 = 60; b = 60; }
        int size = (int)(3.0 / transY);
        if (size < 1) size = 1;
        if (size > 3) size = 3;
        for (int dy = -size; dy <= size; dy++)
            for (int dx = -size; dx <= size; dx++)
                if (dx * dx + dy * dy <= size * size)
                    setPixel(screenX + dx, screenY + dy, r, g2, b);
    }
}

// ==================== 粒子 ====================
void renderParticles() {
    double invDet = 1.0 / (player.planeX * player.dirY - player.dirX * player.planeY);
    for (int i = 0; i < (int)particles.size(); i++) {
        Particle& p = particles[i];
        double spx = p.x - player.x, spy = p.y - player.y;
        double transX = invDet * (player.dirY * spx - player.dirX * spy);
        double transY = invDet * (-player.planeY * spx + player.planeX * spy);
        if (transY <= 0.15) continue;
        int screenX = (int)((PIXEL_W / 2.0) * (1.0 + transX / transY));
        int screenY = PIXEL_H / 2;
        if (screenX < 0 || screenX >= PIXEL_W) continue;
        if (transY >= zBuffer[screenX]) continue;
        int size = (int)(4.0 / transY);
        if (size < 1) size = 1;
        if (size > 4) size = 4;
        double alpha = (double)p.life / p.maxLife;
        if (alpha > 1.0) alpha = 1.0;
        for (int dy = -size; dy <= size; dy++)
            for (int dx = -size; dx <= size; dx++)
                if (dx * dx + dy * dy <= size * size)
                    addPixel(screenX + dx, screenY + dy,
                        (int)(p.r * alpha), (int)(p.g * alpha), (int)(p.b * alpha));
    }
}

// ==================== 枪械 ====================
void renderGun() {
    int cx = PIXEL_W / 2;
    int baseY = PIXEL_H - 1;
    WeaponType wt = (WeaponType)player.currentWeapon;
    int gunH = 14, gunW = 4;
    if (wt == WT_PISTOL) { gunH = 10; gunW = 3; }
    if (wt == WT_RIFLE) { gunH = 16; gunW = 4; }
    if (wt == WT_SNIPER) { gunH = 20; gunW = 3; }
    if (wt == WT_SHOTGUN) { gunH = 14; gunW = 6; }
    for (int y = 0; y < gunH; y++) {
        int py = baseY - y;
        if (py < 0 || py >= PIXEL_H) continue;
        int width = gunW + y / 3;
        for (int x = -width; x <= width; x++) {
            int px = cx + x;
            int r = 55, g = 55, b = 65;
            if (x > -2 && x < 2) { r = 80; g = 80; b = 95; }
            if (abs(x) > width - 2) { r = 30; g = 30; b = 38; }
            setPixel(px, py, r, g, b);
        }
    }
    for (int y = gunH; y < gunH + 4; y++) {
        int py = baseY - y;
        if (py < 0 || py >= PIXEL_H) continue;
        int bw = (wt == WT_SHOTGUN) ? 3 : 2;
        for (int x = -bw; x <= bw; x++) {
            int px = cx + x;
            int r = 40, g = 40, b = 48;
            if (x > -bw + 1 && x < bw - 1) { r = 65; g = 65; b = 78; }
            setPixel(px, py, r, g, b);
        }
    }
    if (muzzleFlash > 0) {
        int flashY = baseY - (gunH + 4);
        double intensity = muzzleFlash / 5.0;
        int size = 4 + muzzleFlash;
        for (int y = -size; y <= size; y++)
            for (int x = -size; x <= size; x++) {
                int d = abs(x) + abs(y);
                if (d <= size) {
                    int br = (int)((size - d) * 55 * intensity);
                    addPixel(cx + x, flashY + y, br, br * 85 / 100, br * 30 / 100);
                }
            }
        for (int y = -2; y <= 2; y++)
            for (int x = -2; x <= 2; x++)
                if (abs(x) + abs(y) <= 2)
                    addPixel(cx + x, flashY + y, 220, 200, 120);
    }
}

// ==================== 准星 ====================
void renderCrosshair() {
    if (!settings.showCrosshair) return;
    int cx = PIXEL_W / 2, cy = PIXEL_H / 2;
    int cr = 0, cg = 255, cb = 0;
    if (hitMarker > 0) { cr = 255; cg = 50; cb = 50; }
    int gap = 4, len = 5;
    for (int i = gap; i < gap + len; i++) {
        setPixel(cx - i, cy, cr, cg, cb);
        setPixel(cx + i, cy, cr, cg, cb);
        setPixel(cx, cy - i, cr, cg, cb);
        setPixel(cx, cy + i, cr, cg, cb);
    }
    setPixel(cx, cy, cr, cg, cb);
}

// ==================== 雷达 ====================
void renderRadar() {
    if (!settings.showRadar) return;
    int radarPxW = RADAR_SIZE * 2, radarPxH = RADAR_SIZE * 2;
    int radarX = PIXEL_W - radarPxW - 4, radarY = 4;
    int centerCX = RADAR_SIZE / 2, centerCY = RADAR_SIZE / 2;
    for (int ry = 0; ry < radarPxH; ry++)
        for (int rx = 0; rx < radarPxW; rx++) {
            int px = radarX + rx, py = radarY + ry;
            if (px < 0 || px >= PIXEL_W || py < 0 || py >= PIXEL_H) continue;
            pxBuf[py][px][0] /= 5; pxBuf[py][px][1] /= 5; pxBuf[py][px][2] /= 5;
        }
    for (int i = 0; i < radarPxW; i++) {
        int px = radarX + i;
        if (px < 0 || px >= PIXEL_W) continue;
        setPixel(px, radarY, 80, 200, 80);
        setPixel(px, radarY + radarPxH - 1, 80, 200, 80);
    }
    for (int i = 0; i < radarPxH; i++) {
        int py = radarY + i;
        if (py < 0 || py >= PIXEL_H) continue;
        setPixel(radarX, py, 80, 200, 80);
        setPixel(radarX + radarPxW - 1, py, 80, 200, 80);
    }
    int playerCX = (int)player.x, playerCY = (int)player.y;
    for (int cy = 0; cy < RADAR_SIZE; cy++)
        for (int cx = 0; cx < RADAR_SIZE; cx++) {
            int mx = playerCX + (cx - centerCX);
            int my = playerCY + (cy - centerCY);
            if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) continue;
            int px0 = radarX + cx * 2, py0 = radarY + cy * 2;
            if (worldMap[my][mx] > 0) {
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++)
                        setPixel(px0 + dx, py0 + dy, 100, 100, 110);
            }
            else {
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++)
                        setPixel(px0 + dx, py0 + dy, 15, 35, 20);
            }
        }
    for (int i = 0; i < (int)allies.size(); i++) {
        if (!allies[i].alive) continue;
        int rcx = (int)allies[i].x - playerCX + centerCX;
        int rcy = (int)allies[i].y - playerCY + centerCY;
        if (rcx < 0 || rcx >= RADAR_SIZE || rcy < 0 || rcy >= RADAR_SIZE) continue;
        int px0 = radarX + rcx * 2, py0 = radarY + rcy * 2;
        for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++)
                setPixel(px0 + dx, py0 + dy, 60, 150, 255);
    }
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (!enemies[i].alive) continue;
        int rcx = (int)enemies[i].x - playerCX + centerCX;
        int rcy = (int)enemies[i].y - playerCY + centerCY;
        if (rcx < 0 || rcx >= RADAR_SIZE || rcy < 0 || rcy >= RADAR_SIZE) continue;
        int px0 = radarX + rcx * 2, py0 = radarY + rcy * 2;
        int rr = 255, gg = 255, bb = 255;
        switch (enemies[i].type) {
        case ET_BOSS:   rr = 255; gg = 100; bb = 100; break;
        case ET_HEALER: rr = 100; gg = 255; bb = 140; break;
        case ET_GHOST:  rr = 180; gg = 180; bb = 255; break;
        default: rr = 255; gg = 255; bb = 255;
        }
        for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++)
                setPixel(px0 + dx, py0 + dy, rr, gg, bb);
    }
    {
        int px0 = radarX + centerCX * 2, py0 = radarY + centerCY * 2;
        for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++)
                setPixel(px0 + dx, py0 + dy, 80, 255, 80);
    }
    for (double t = 1.0; t < RADAR_SIZE * 0.6; t += 0.5) {
        int fx = (int)(player.x + player.dirX * t);
        int fy = (int)(player.y + player.dirY * t);
        int rcx = fx - playerCX + centerCX;
        int rcy = fy - playerCY + centerCY;
        if (rcx < 0 || rcx >= RADAR_SIZE || rcy < 0 || rcy >= RADAR_SIZE) break;
        int px0 = radarX + rcx * 2, py0 = radarY + rcy * 2;
        int br = (int)(180 - t * 20);
        if (br < 40) br = 40;
        setPixel(px0, py0, br, 255, br);
        setPixel(px0 + 1, py0, br, 255, br);
    }
}

// ==================== 屏幕色调 ====================
void applyScreenTint() {
    if (hurtFlash > 0) {
        double strength = hurtFlash / 5.0 * 0.55;
        for (int y = 0; y < PIXEL_H; y++) {
            double edge = 1.0;
            if (y < PIXEL_H / 4) edge = 1.0 + (PIXEL_H / 4 - y) / (double)(PIXEL_H / 4);
            if (y > PIXEL_H * 3 / 4) edge = 1.0 + (y - PIXEL_H * 3 / 4) / (double)(PIXEL_H / 4);
            double s = strength * (edge > 1.4 ? 1.4 : edge);
            if (s > 0.7) s = 0.7;
            for (int x = 0; x < PIXEL_W; x++) {
                int r = pxBuf[y][x][0], g = pxBuf[y][x][1], b = pxBuf[y][x][2];
                pxBuf[y][x][0] = clamp255(r + (int)(s * 180));
                pxBuf[y][x][1] = clamp255(g - (int)(s * 60));
                pxBuf[y][x][2] = clamp255(b - (int)(s * 60));
            }
        }
    }
    if (killFlash > 0) {
        double strength = killFlash / 4.0 * 0.25;
        for (int y = 0; y < PIXEL_H; y++)
            for (int x = 0; x < PIXEL_W; x++) {
                int r = pxBuf[y][x][0], g = pxBuf[y][x][1], b = pxBuf[y][x][2];
                pxBuf[y][x][0] = clamp255(r + (int)(strength * 120));
                pxBuf[y][x][1] = clamp255(g + (int)(strength * 100));
                pxBuf[y][x][2] = clamp255(b - (int)(strength * 40));
            }
    }
    if (player.hp <= 30 && player.hp > 0 && gameState == GS_PLAYING) {
        double pulse = (sin(GetTickCount() * 0.005) + 1.0) * 0.5;
        double strength = pulse * 0.18;
        for (int y = 0; y < PIXEL_H; y++)
            for (int x = 0; x < PIXEL_W; x++)
                pxBuf[y][x][0] = clamp255(pxBuf[y][x][0] + (int)(strength * 100));
    }
}

// ==================== 渲染主入口 ====================
void render() {
    renderBackground();
    renderWalls();
    renderEntities();
    renderDeathAnims();
    renderGrenades();
    renderParticles();
    renderMuzzleFlashes();
    renderGun();
    renderCrosshair();
    renderRadar();
    applyScreenTint();
}

// ==================== 像素→字符 ====================
void flushToScreen(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int cy = 0; cy < SCREEN_H; cy++)
        for (int cx = 0; cx < SCREEN_W; cx++) {
            int topY = cy * 2, botY = cy * 2 + 1;
            int fg = rgbToIndex(pxBuf[topY][cx][0], pxBuf[topY][cx][1], pxBuf[topY][cx][2]);
            int bgc = rgbToIndex(pxBuf[botY][cx][0], pxBuf[botY][cx][1], pxBuf[botY][cx][2]);
            screenBuf[cy][cx].Char.UnicodeChar = 0x2580;
            screenBuf[cy][cx].Attributes = (WORD)(fg | (bgc << 4));
        }
}

// ==================== 文字 ====================
void drawText(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W], int x, int y,
    const std::wstring& text, int fgColor, int bgColor) {
    for (int i = 0; i < (int)text.size(); i++) {
        int px = x + i;
        if (px < 0 || px >= SCREEN_W || y < 0 || y >= SCREEN_H) continue;
        screenBuf[y][px].Char.UnicodeChar = text[i];
        screenBuf[y][px].Attributes = (WORD)(fgColor | (bgColor << 4));
    }
}
void drawTextCentered(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W], int y,
    const std::wstring& text, int fgColor, int bgColor) {
    int x = (SCREEN_W - (int)text.size()) / 2;
    drawText(screenBuf, x, y, text, fgColor, bgColor);
}

// ==================== 界面：主菜单 ====================
void renderMainMenu(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"==============================================", 11, 0);
    drawTextCentered(screenBuf, 3, L"||                                          ||", 11, 0);
    drawTextCentered(screenBuf, 4, L"||          Z E R O   T E R M I N A L       ||", 10, 0);
    drawTextCentered(screenBuf, 5, L"||         C O N S O L E   S T R I K E      ||", 11, 0);
    drawTextCentered(screenBuf, 6, L"||                                          ||", 11, 0);
    drawTextCentered(screenBuf, 7, L"==============================================", 11, 0);

    bool hasSaveFile = hasSave();
    int y = 10;
    if (hasSaveFile) {
        drawTextCentered(screenBuf, y, L"[ Enter ]  \u7EE7\u7EED\u6E38\u620F", 14, 0);
        y += 2;
    }
    drawTextCentered(screenBuf, y, L"[ M ]  \u4EFB\u52A1\u6A21\u5F0F (100 \u5173)", 10, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ E ]  \u65E0\u5C3D\u6A21\u5F0F", 12, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ N ]  \u65B0\u6E38\u620F\uFF08\u9009\u96BE\u5EA6\uFF09", 10, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ T ]  \u65B0\u624B\u6559\u7A0B", 14, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ A ]  \u6210\u5C31", 14, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ L ]  \u6392\u884C\u699C", 14, 0); y += 2;
    drawTextCentered(screenBuf, y, L"[ Esc ]  \u9000\u51FA", 12, 0);

    drawTextCentered(screenBuf, 26, L"\u63D0\u793A\uFF1A\u6E38\u620F\u4F1A\u81EA\u52A8\u4FDD\u5B58", 8, 0);
}

// ==================== 难度选择 ====================
void renderDiffSelect(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 3, L"========== \u9009\u62E9\u96BE\u5EA6 ==========", 14, 0);
    drawTextCentered(screenBuf, 6, L"[ 1 ]  \u7B80\u5355", 10, 0);
    drawTextCentered(screenBuf, 7, L"   \u654C\u4EBA\u4F24\u5BB3\u4F4E\uFF0C\u5237\u65B0\u6162\uFF0C\u521D\u59CB HP \u9AD8", 8, 0);
    drawTextCentered(screenBuf, 10, L"[ 2 ]  \u666E\u901A", 14, 0);
    drawTextCentered(screenBuf, 11, L"   \u6807\u51C6\u96BE\u5EA6\uFF0C\u63A8\u8350", 8, 0);
    drawTextCentered(screenBuf, 14, L"[ 3 ]  \u5730\u72F1", 12, 0);
    drawTextCentered(screenBuf, 15, L"   \u654C\u4EBA\u4F24\u5BB3\u9AD8\uFF0C\u5237\u65B0\u5FEB\uFF0C\u521D\u59CB HP \u4F4E", 8, 0);
    drawTextCentered(screenBuf, 20, L"[ Esc ]  \u8FD4\u56DE", 8, 0);
}

// ==================== 任务选择 ====================
void renderMissionSelect(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 1, L"========== \u4EFB\u52A1\u9009\u62E9 (100 \u5173) ==========", 14, 0);

    int start = missionSelectIndex - 5;
    if (start < 0) start = 0;
    if (start > TOTAL_MISSIONS - 11) start = TOTAL_MISSIONS - 11;
    if (start < 0) start = 0;

    for (int i = 0; i < 11 && start + i < TOTAL_MISSIONS; i++) {
        int idx = start + i;
        int y = 3 + i * 2;
        std::wstring line = MISSIONS[idx].title;
        if (idx == missionSelectIndex) {
            drawTextCentered(screenBuf, y, L"> " + line + L" <", 14, 0);
        }
        else if (idx <= missionsCompleted) {
            drawTextCentered(screenBuf, y, L"  " + line + L"  ", 10, 0);
        }
        else {
            drawTextCentered(screenBuf, y, L"  " + line + L"  ", 8, 0);
        }
    }

    drawTextCentered(screenBuf, 26, L"[ \u2191\u2193 ] \u9009\u62E9   [ Enter ] \u5F00\u59CB   [ Esc ] \u8FD4\u56DE", 8, 0);
    wchar_t buf[64];
    swprintf_s(buf, L"\u5DF2\u5B8C\u6210: %d / 100", missionsCompleted);
    drawTextCentered(screenBuf, 28, buf, 11, 0);
}

// ==================== 教程 ====================
void renderTutorial(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    if (tutorialPage == 0) {
        drawTextCentered(screenBuf, 2, L"========== \u65B0\u624B\u6559\u7A0B (1/3) ==========", 14, 0);
        drawTextCentered(screenBuf, 5, L"\u79FB\u52A8\uFF1AW A S D", 10, 0);
        drawTextCentered(screenBuf, 7, L"\u8F6C\u5411\uFF1AQ / E \u6216 \u2190 / \u2192", 10, 0);
        drawTextCentered(screenBuf, 9, L"\u5C04\u51FB\uFF1A\u7A7A\u683C\u952E", 10, 0);
        drawTextCentered(screenBuf, 11, L"\u6362\u5F39\uFF1AR \u952E", 10, 0);
        drawTextCentered(screenBuf, 13, L"\u5207\u6362\u6B66\u5668\uFF1A1 \u624B\u67AA  2 \u6B65\u67AA  3 \u72D9\u51FB  4 \u6563\u5F39", 14, 0);
        drawTextCentered(screenBuf, 15, L"\u624B\u96F7\uFF1AG \u952E", 12, 0);
    }
    else if (tutorialPage == 1) {
        drawTextCentered(screenBuf, 2, L"========== \u65B0\u624B\u6559\u7A0B (2/3) ==========", 14, 0);
        drawTextCentered(screenBuf, 5, L"\u654C\u4EBA\u7C7B\u578B\uFF1A", 10, 0);
        drawTextCentered(screenBuf, 7, L"\u767D\u8272 = \u666E\u901A    \u6A59\u8272 = \u5FEB\u901F", 8, 0);
        drawTextCentered(screenBuf, 9, L"\u7070\u8272 = \u91CD\u88C5    \u7EFF\u8272 = \u72D9\u51FB", 8, 0);
        drawTextCentered(screenBuf, 11, L"\u7EA2\u8272 = BOSS    \u9752\u8272 = \u6CBB\u7597\u5175", 8, 0);
        drawTextCentered(screenBuf, 13, L"\u6DE1\u84DD = \u9690\u8EAB\u5175", 8, 0);
        drawTextCentered(screenBuf, 15, L"\u84DD\u8272 = \u961F\u53CB", 11, 0);
    }
    else {
        drawTextCentered(screenBuf, 2, L"========== \u65B0\u624B\u6559\u7A0B (3/3) ==========", 14, 0);
        drawTextCentered(screenBuf, 5, L"\u4EFB\u52A1\u6A21\u5F0F\uFF1A100 \u5173\uFF0C\u9010\u6E10\u53D8\u96BE", 10, 0);
        drawTextCentered(screenBuf, 7, L"\u65E0\u5C3D\u6A21\u5F0F\uFF1A\u65E0\u9650\u6CE2\u6B21\uFF0C\u770B\u4F60\u80FD\u6491\u591A\u4E45", 12, 0);
        drawTextCentered(screenBuf, 9, L"\u6BCF\u51FB\u6740 4 \u4E2A\u654C\u4EBA\u63D0\u5347\u6CE2\u6B21", 12, 0);
        drawTextCentered(screenBuf, 11, L"\u6BCF 10 \u6CE2\u51FA\u4E00\u4E2A BOSS", 12, 0);
        drawTextCentered(screenBuf, 13, L"\u5B8C\u6210\u6210\u5C31\u53EF\u4EE5\u5728\u4E3B\u83DC\u5355 [ A ] \u67E5\u770B", 14, 0);
        drawTextCentered(screenBuf, 16, L"Good luck, soldier.", 10, 0);
    }
    drawTextCentered(screenBuf, 22, L"[ \u7A7A\u683C / Enter ]  \u4E0B\u4E00\u9875", 14, 0);
    drawTextCentered(screenBuf, 24, L"[ Esc ]  \u8DF3\u8FC7\u6559\u7A0B", 8, 0);
}

// ==================== 暂停菜单 ====================
void renderPauseMenu(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"========== \u6682\u505C / \u8BBE\u7F6E ==========", 14, 0);
    const wchar_t* items[] = {
        L"\u7EE7\u7EED\u6E38\u620F", L"\u663E\u793A\u96F7\u8FBE",
        L"\u663E\u793A\u51C6\u661F", L"\u663E\u793A FPS",
        L"\u97F3\u6548\u5F00\u5173", L"\u9000\u51FA\u5230\u4E3B\u83DC\u5355"
    };
    int itemCount = 6;
    for (int i = 0; i < itemCount; i++) {
        int y = 5 + i * 2;
        std::wstring line;
        bool isOn = false;
        if (i == 1) isOn = settings.showRadar;
        else if (i == 2) isOn = settings.showCrosshair;
        else if (i == 3) isOn = settings.showFps;
        else if (i == 4) isOn = settings.soundOn;
        if (i >= 1 && i <= 4) line = std::wstring(isOn ? L"[X] " : L"[ ] ") + items[i];
        else line = std::wstring(L"    ") + items[i];
        if (i == pauseMenuIndex) drawTextCentered(screenBuf, y, L"> " + line + L" <", 14, 0);
        else drawTextCentered(screenBuf, y, L"  " + line + L"  ", 10, 0);
    }
    drawTextCentered(screenBuf, 19, L"[ \u2191\u2193 ]  \u9009\u62E9    [ Enter ]  \u5207\u6362/\u786E\u8BA4", 8, 0);
    drawTextCentered(screenBuf, 21, L"[ Esc ]  \u7EE7\u7EED\u6E38\u620F", 8, 0);
    drawTextCentered(screenBuf, 23, L"\u6E38\u620F\u4F1A\u81EA\u52A8\u4FDD\u5B58", 8, 0);
}

// ==================== 游戏结束 ====================
void renderGameOver(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 4;
        }
    drawTextCentered(screenBuf, 4, L"==============================================", 12, 0);
    drawTextCentered(screenBuf, 5, L"||          Z E R O   T E R M I N A L       ||", 12, 0);
    drawTextCentered(screenBuf, 6, L"||              G A M E   O V E R           ||", 12, 0);
    drawTextCentered(screenBuf, 7, L"==============================================", 12, 0);

    wchar_t buf[128];
    swprintf_s(buf, L"\u6700\u7EC8\u5F97\u5206: %d", player.score);
    drawTextCentered(screenBuf, 10, buf, 14, 0);
    if (endlessMode) {
        swprintf_s(buf, L"\u51FB\u6740\u6570: %d    \u6CE2\u6B21: %d    [\u65E0\u5C3D]",
            totalKills, endlessWave);
    }
    else {
        swprintf_s(buf, L"\u51FB\u6740\u6570: %d    \u4EFB\u52A1: %d / 100",
            totalKills, currentLevel + 1);
    }
    drawTextCentered(screenBuf, 12, buf, 14, 0);
    swprintf_s(buf, L"\u96BE\u5EA6: %s", diffCfg->name);
    drawTextCentered(screenBuf, 14, buf, 14, 0);
    drawTextCentered(screenBuf, 18, L"[ Enter ]  \u91CD\u65B0\u5F00\u59CB", 10, 0);
    drawTextCentered(screenBuf, 20, L"[ L ]  \u6392\u884C\u699C", 14, 0);
    drawTextCentered(screenBuf, 22, L"[ Esc ]  \u8FD4\u56DE\u4E3B\u83DC\u5355", 8, 0);
}

// ==================== 成就 ====================
void renderAchievements(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"========== \u6210\u5C31 ==========", 14, 0);
    int y = 5;
    for (int i = 0; i < ACHIEVEMENT_COUNT; i++) {
        wchar_t line[128];
        swprintf_s(line, L"%s  %s   %s",
            ACHIEVEMENTS[i].unlocked ? L"[\u2714]" : L"[ ]",
            ACHIEVEMENTS[i].name, ACHIEVEMENTS[i].desc);
        drawTextCentered(screenBuf, y, line,
            ACHIEVEMENTS[i].unlocked ? 10 : 8, 0);
        y += 2;
    }
    drawTextCentered(screenBuf, 25, L"[ Esc ]  \u8FD4\u56DE", 8, 0);
}

// ==================== 排行榜 ====================
void renderLeaderboard(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"========== \u6392\u884C\u699C TOP 5 ==========", 14, 0);
    if (leaderboardCount == 0) {
        drawTextCentered(screenBuf, 10, L"\u6682\u65E0\u8BB0\u5F55", 8, 0);
    }
    else {
        drawTextCentered(screenBuf, 5, L"\u540D\u6B21    \u5F97\u5206    \u51FB\u6740    \u96BE\u5EA6    \u5173\u5361", 11, 0);
        for (int i = 0; i < leaderboardCount; i++) {
            wchar_t line[128];
            swprintf_s(line, L" #%d      %d      %d      %s      %d",
                i + 1, leaderboard[i].score, leaderboard[i].kills,
                DIFFS[leaderboard[i].difficulty].name,
                leaderboard[i].level + 1);
            drawTextCentered(screenBuf, 7 + i * 2, line, 10, 0);
        }
    }
    drawTextCentered(screenBuf, 25, L"[ Esc ]  \u8FD4\u56DE", 8, 0);
}

// ==================== 关卡完成 ====================
void renderLevelComplete(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 4, L"==============================================", 10, 0);
    drawTextCentered(screenBuf, 5, L"||              \u4EFB\u52A1\u5B8C\u6210                  ||", 10, 0);
    drawTextCentered(screenBuf, 6, L"||                                          ||", 10, 0);
    wchar_t line[128];
    swprintf_s(line, L"||              %s                   ||", MISSIONS[currentLevel].title);
    drawTextCentered(screenBuf, 7, line, 10, 0);
    drawTextCentered(screenBuf, 8, L"==============================================", 10, 0);

    swprintf_s(line, L"\u5F53\u524D\u5F97\u5206: %d", player.score);
    drawTextCentered(screenBuf, 11, line, 14, 0);
    swprintf_s(line, L"\u672C\u5173\u51FB\u6740: %d", totalKills - levelKillsAtStart);
    drawTextCentered(screenBuf, 13, line, 14, 0);
    swprintf_s(line, L"\u5F53\u524D HP: %d / %d", player.hp, player.maxHp);
    drawTextCentered(screenBuf, 15, line, 14, 0);
    swprintf_s(line, L"\u8FDB\u5EA6: %d / 100", currentLevel + 1);
    drawTextCentered(screenBuf, 17, line, 11, 0);

    drawTextCentered(screenBuf, 21, L"\u6309 [ \u7A7A\u683C ] \u67E5\u770B\u8FC7\u5173\u5BF9\u8BDD", 10, 0);
}

// ==================== 任务简报 ====================
void renderLevelBrief(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"==============================================", 11, 0);
    drawTextCentered(screenBuf, 3, MISSIONS[currentLevel].title, 14, 0);
    drawTextCentered(screenBuf, 4, L"==============================================", 11, 0);

    drawTextCentered(screenBuf, 7, MISSIONS[currentLevel].target, 12, 0);
    drawTextCentered(screenBuf, 10, MISSION_BRIEFS[currentLevel], 10, 0);

    wchar_t line[128];
    swprintf_s(line, L"\u8FDB\u5EA6: %d / 100", currentLevel + 1);
    drawTextCentered(screenBuf, 13, line, 11, 0);

    drawTextCentered(screenBuf, 16, L"\u63D0\u793A\uFF1A\u6CE8\u610F\u4FDD\u6301\u8DDD\u79BB\uFF0C\u5584\u7528\u624B\u96F7\u3002", 8, 0);
    drawTextCentered(screenBuf, 18, L"\u795D\u4F60\u597D\u8FD0\uFF0C\u58EB\u5175\u3002", 10, 0);
    drawTextCentered(screenBuf, 22, L"[ \u7A7A\u683C ]  \u5F00\u59CB\u4EFB\u52A1", 14, 0);
}

// ==================== 过关对话 ====================
void renderLevelDialog(CHAR_INFO screenBuf[SCREEN_H][SCREEN_W]) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            screenBuf[y][x].Char.UnicodeChar = L' ';
            screenBuf[y][x].Attributes = 0;
        }
    drawTextCentered(screenBuf, 2, L"==============================================", 10, 0);

    wchar_t title[64];
    swprintf_s(title, L"\u4EFB\u52A1 %d \u5B8C\u6210", currentLevel + 1);
    drawTextCentered(screenBuf, 3, title, 10, 0);
    drawTextCentered(screenBuf, 4, L"==============================================", 10, 0);

    drawTextCentered(screenBuf, 8, L"\u6307\u6325\u5B98\uFF1A\u5E72\u5F97\u6F02\u4EAE\uFF0C\u58EB\u5175\uFF01", 10, 0);
    drawTextCentered(screenBuf, 10, MISSIONS[currentLevel].target, 10, 0);
    drawTextCentered(screenBuf, 12, L"\u4EFB\u52A1\uFF1A\u51C6\u5907\u8FCE\u63A5\u4E0B\u4E00\u6CE2\u8FDB\u653B\u3002", 10, 0);
    drawTextCentered(screenBuf, 14, L"\u4FDD\u6301\u8B66\u60D5\u3002", 10, 0);

    wchar_t buf[128];
    swprintf_s(buf, L"\u5F53\u524D\u5F97\u5206: %d", player.score);
    drawTextCentered(screenBuf, 17, buf, 14, 0);
    swprintf_s(buf, L"\u51FB\u6740\u6570: %d", totalKills);
    drawTextCentered(screenBuf, 19, buf, 14, 0);

    if (currentLevel >= TOTAL_MISSIONS - 1) {
        drawTextCentered(screenBuf, 22, L"\u5168\u90E8 100 \u5173\u5B8C\u6210\uFF01", 10, 0);
        drawTextCentered(screenBuf, 24, L"[ \u7A7A\u683C ]  \u8FD4\u56DE\u4E3B\u83DC\u5355", 14, 0);
    }
    else {
        drawTextCentered(screenBuf, 24, L"[ \u7A7A\u683C ]  \u8FDB\u5165\u4E0B\u4E00\u5173", 14, 0);
    }
}
// ==================== 主函数 ====================
int main() {
    SetConsoleTitleW(L"ZERO TERMINAL - Console Strike");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inMode;
    GetConsoleMode(hIn, &inMode);
    SetConsoleMode(hIn, inMode & ~ENABLE_QUICK_EDIT_MODE);

    SMALL_RECT tinyRect = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &tinyRect);
    COORD bufSize = { (SHORT)SCREEN_W, (SHORT)SCREEN_H };
    SetConsoleScreenBufferSize(hOut, bufSize);
    SMALL_RECT winRect = { 0, 0, (SHORT)(SCREEN_W - 1), (SHORT)(SCREEN_H - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &winRect);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);

    // 字体设置：按优先级尝试
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 18;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    const wchar_t* preferredFonts[] = {
        L"NSimSun", L"SimSun", L"Microsoft YaHei", L"Microsoft YaHei UI", L"Consolas"
    };
    CONSOLE_FONT_INFOEX check;
    check.cbSize = sizeof(check);
    for (size_t fi = 0; fi < sizeof(preferredFonts) / sizeof(preferredFonts[0]); fi++) {
        wcscpy_s(cfi.FaceName, preferredFonts[fi]);
        SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
        if (GetCurrentConsoleFontEx(hOut, FALSE, &check)) {
            if (wcsstr(check.FaceName, preferredFonts[fi]) != NULL ||
                wcscmp(check.FaceName, preferredFonts[fi]) == 0) {
                break;
            }
        }
    }

    DWORD outMode = 0;
    if (GetConsoleMode(hOut, &outMode))
        SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::wstring loadingMsg = L"\u6B63\u5728\u8FDE\u63A5\u7F51\u7EDC\u52A0\u8F7D\u8D34\u56FE...";
    DWORD written;
    WriteConsoleW(hOut, loadingMsg.c_str(), (DWORD)loadingMsg.size(), &written, NULL);

    srand((unsigned)GetTickCount());
    initMap();
    loadAllTextures();
    loadLeaderboard();

    COORD topLeft = { 0, 0 };
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterW(hOut, L' ', cells, topLeft, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, topLeft, &written);
    SetConsoleCursorPosition(hOut, topLeft);

    gameState = GS_MAIN_MENU;

    SaveData saveData;
    bool hasSaveFile = hasSave();

    CHAR_INFO screenBuf[SCREEN_H][SCREEN_W];
    COORD writeSize = { (SHORT)SCREEN_W, (SHORT)SCREEN_H };
    COORD writeCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(SCREEN_W - 1), (SHORT)(SCREEN_H - 1) };

    const int TARGET_MS = 33;
    int statusTimer = 120;
    int autoSaveTimer = 0;

    while (true) {
        auto frameStart = std::chrono::steady_clock::now();
        keys.update();

        // ===== 全局 Esc =====
        if (keys.pressed[VK_ESCAPE]) {
            if (gameState == GS_PLAYING) {
                gameState = GS_PAUSED;
                pauseMenuIndex = 0;
                saveGame();
                playSound(SND_MENU_CLICK);
            }
            else if (gameState == GS_PAUSED) {
                gameState = GS_PLAYING;
                playSound(SND_MENU_CLICK);
            }
            else if (gameState == GS_MAIN_MENU) {
                break;
            }
            else if (gameState == GS_DIFF_SELECT ||
                gameState == GS_TUTORIAL ||
                gameState == GS_ACHIEVEMENTS ||
                gameState == GS_LEADERBOARD ||
                gameState == GS_MISSION_SELECT) {
                gameState = GS_MAIN_MENU;
                playSound(SND_MENU_CLICK);
            }
            else if (gameState == GS_GAME_OVER) {
                gameState = GS_MAIN_MENU;
                playSound(SND_MENU_CLICK);
            }
        }

        // ===== 主菜单 =====
        if (gameState == GS_MAIN_MENU) {
            if (keys.pressed[VK_RETURN] && hasSaveFile) {
                if (loadGame(saveData)) {
                    restoreFromSave(saveData);
                    gameState = endlessMode ? GS_PLAYING : GS_LEVEL_BRIEF;
                    playSound(SND_MENU_CLICK);
                }
            }
            if (keys.pressed['M']) {
                gameState = GS_MISSION_SELECT;
                missionSelectIndex = 0;
                if (missionSelectIndex > missionsCompleted) missionSelectIndex = missionsCompleted;
                playSound(SND_MENU_CLICK);
            }
            if (keys.pressed['E']) {
                currentDiff = DIFF_HELL;
                diffCfg = &DIFFS[currentDiff];
                resetGame();
                endlessMode = true;
                endlessWave = 0;
                gameState = GS_PLAYING;
                playSound(SND_MENU_CLICK);
            }
            if (keys.pressed['N']) { gameState = GS_DIFF_SELECT; playSound(SND_MENU_CLICK); }
            if (keys.pressed['T']) { gameState = GS_TUTORIAL; tutorialPage = 0; playSound(SND_MENU_CLICK); }
            if (keys.pressed['A']) { gameState = GS_ACHIEVEMENTS; playSound(SND_MENU_CLICK); }
            if (keys.pressed['L']) { gameState = GS_LEADERBOARD; playSound(SND_MENU_CLICK); }
        }
        // ===== 任务选择 =====
        else if (gameState == GS_MISSION_SELECT) {
            if (keys.pressed[VK_UP]) {
                missionSelectIndex--;
                if (missionSelectIndex < 0) missionSelectIndex = 0;
                playSound(SND_MENU_CLICK);
            }
            if (keys.pressed[VK_DOWN]) {
                missionSelectIndex++;
                if (missionSelectIndex > missionsCompleted) missionSelectIndex = missionsCompleted;
                if (missionSelectIndex >= TOTAL_MISSIONS) missionSelectIndex = TOTAL_MISSIONS - 1;
                playSound(SND_MENU_CLICK);
            }
            if (keys.pressed[VK_RETURN]) {
                currentLevel = missionSelectIndex;
                endlessMode = false;
                resetGame();
                currentLevel = missionSelectIndex;
                gameState = GS_LEVEL_BRIEF;
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 难度选择 =====
        else if (gameState == GS_DIFF_SELECT) {
            int newDiff = -1;
            if (keys.pressed['1']) newDiff = DIFF_EASY;
            if (keys.pressed['2']) newDiff = DIFF_NORMAL;
            if (keys.pressed['3']) newDiff = DIFF_HELL;
            if (newDiff >= 0) {
                currentDiff = (Difficulty)newDiff;
                diffCfg = &DIFFS[currentDiff];
                resetGame();
                endlessMode = false;
                currentLevel = 0;
                gameState = GS_LEVEL_BRIEF;
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 教程 =====
        else if (gameState == GS_TUTORIAL) {
            if (keys.pressed[VK_SPACE] || keys.pressed[VK_RETURN]) {
                tutorialPage++;
                if (tutorialPage >= TUTORIAL_PAGES) gameState = GS_DIFF_SELECT;
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 任务简报 =====
        else if (gameState == GS_LEVEL_BRIEF) {
            if (keys.pressed[VK_SPACE] || keys.pressed[VK_RETURN]) {
                gameState = GS_PLAYING;
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 游戏中 =====
        else if (gameState == GS_PLAYING) {
            if (!player.alive) {
                gameState = GS_GAME_OVER;
                saveGame();
            }
            else {
                update();
            }
        }
        // ===== 暂停 =====
        else if (gameState == GS_PAUSED) {
            if (keys.pressed[VK_UP]) { pauseMenuIndex--; if (pauseMenuIndex < 0) pauseMenuIndex = 5; playSound(SND_MENU_CLICK); }
            if (keys.pressed[VK_DOWN]) { pauseMenuIndex++; if (pauseMenuIndex > 5) pauseMenuIndex = 0; playSound(SND_MENU_CLICK); }
            if (keys.pressed[VK_RETURN]) {
                switch (pauseMenuIndex) {
                case 0: gameState = GS_PLAYING; break;
                case 1: settings.showRadar = !settings.showRadar; break;
                case 2: settings.showCrosshair = !settings.showCrosshair; break;
                case 3: settings.showFps = !settings.showFps; break;
                case 4: settings.soundOn = !settings.soundOn; soundEnabled = settings.soundOn != 0; break;
                case 5: saveGame(); gameState = GS_MAIN_MENU; break;
                }
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 游戏结束 =====
        else if (gameState == GS_GAME_OVER) {
            if (keys.pressed[VK_RETURN]) {
                resetGame();
                endlessMode = false;
                currentLevel = 0;
                gameState = GS_LEVEL_BRIEF;
                playSound(SND_MENU_CLICK);
            }
            if (keys.pressed['L']) { gameState = GS_LEADERBOARD; playSound(SND_MENU_CLICK); }
        }
        // ===== 关卡完成 =====
        else if (gameState == GS_LEVEL_COMPLETE) {
            if (keys.pressed[VK_SPACE] || keys.pressed[VK_RETURN]) {
                gameState = GS_LEVEL_DIALOG;
                playSound(SND_MENU_CLICK);
            }
        }
        // ===== 过关对话 =====
        else if (gameState == GS_LEVEL_DIALOG) {
            if (keys.pressed[VK_SPACE] || keys.pressed[VK_RETURN]) {
                if (endlessMode) {
                    gameState = GS_PLAYING;
                }
                else if (currentLevel >= TOTAL_MISSIONS - 1) {
                    // 通关
                    if (missionsCompleted < TOTAL_MISSIONS)
                        missionsCompleted = TOTAL_MISSIONS;
                    gameState = GS_MAIN_MENU;
                    playSound(SND_LEVEL_UP);
                }
                else {
                    if (currentLevel + 1 > missionsCompleted)
                        missionsCompleted = currentLevel + 1;
                    currentLevel++;
                    enemies.clear(); allies.clear();
                    grenades.clear(); particles.clear();
                    deathAnims.clear(); muzzleFlashes.clear();
                    for (int i = 0; i < 4; i++) spawnEnemy();
                    spawnAlly(); spawnAlly();
                    gameState = GS_LEVEL_BRIEF;
                }
                playSound(SND_MENU_CLICK);
            }
        }

        // ===== 渲染 =====
        if (gameState == GS_PLAYING || gameState == GS_PAUSED) {
            render();
            flushToScreen(screenBuf);
        }
        else {
            for (int y = 0; y < SCREEN_H; y++)
                for (int x = 0; x < SCREEN_W; x++) {
                    screenBuf[y][x].Char.UnicodeChar = L' ';
                    screenBuf[y][x].Attributes = 0;
                }
        }

        // ===== 界面覆盖层 =====
        if (gameState == GS_MAIN_MENU) {
            renderMainMenu(screenBuf);
        }
        else if (gameState == GS_MISSION_SELECT) {
            renderMissionSelect(screenBuf);
        }
        else if (gameState == GS_DIFF_SELECT) {
            renderDiffSelect(screenBuf);
        }
        else if (gameState == GS_TUTORIAL) {
            renderTutorial(screenBuf);
        }
        else if (gameState == GS_PAUSED) {
            renderPauseMenu(screenBuf);
        }
        else if (gameState == GS_GAME_OVER) {
            renderGameOver(screenBuf);
        }
        else if (gameState == GS_ACHIEVEMENTS) {
            renderAchievements(screenBuf);
        }
        else if (gameState == GS_LEADERBOARD) {
            renderLeaderboard(screenBuf);
        }
        else if (gameState == GS_LEVEL_COMPLETE) {
            renderLevelComplete(screenBuf);
        }
        else if (gameState == GS_LEVEL_BRIEF) {
            renderLevelBrief(screenBuf);
        }
        else if (gameState == GS_LEVEL_DIALOG) {
            renderLevelDialog(screenBuf);
        }
        else if (gameState == GS_PLAYING) {
            // HUD
            {
                int radarPxW = RADAR_SIZE * 2;
                int radarX = PIXEL_W - radarPxW - 4;
                int radarY = 4;
                int radarTopChar = radarY / 2 - 1;
                int radarLeftChar = radarX / 2;
                if (radarTopChar >= 0)
                    drawText(screenBuf, radarLeftChar, radarTopChar, L" R A D A R ", 10, 0);
            }
            {
                wchar_t buf[220];

                swprintf_s(buf, L" HP: %d", player.hp);
                int hpColor = (player.hp > 60) ? 10 : (player.hp > 30 ? 14 : 12);
                drawText(screenBuf, 1, SCREEN_H - 1, buf, hpColor, 0);

                WeaponDef& w = WEAPONS[player.currentWeapon];
                swprintf_s(buf, L" %s: %d/%d  [G] x%d",
                    w.name, player.magAmmo[player.currentWeapon],
                    w.magSize, player.grenades);
                drawText(screenBuf, 14, SCREEN_H - 1, buf, 15, 0);
                if (reloading)
                    drawText(screenBuf, 46, SCREEN_H - 1, L" RELOADING... ", 14, 0);
                swprintf_s(buf, L" SCORE: %d ", player.score);
                drawText(screenBuf, SCREEN_W - 20, SCREEN_H - 1, buf, 11, 0);

                int aliveE = (int)enemies.size();
                int aliveA = (int)allies.size();
                if (endlessMode) {
                    swprintf_s(buf, L" E:%d A:%d WAVE:%d KILL:%d [ENDLESS]",
                        aliveE, aliveA, endlessWave, totalKills);
                }
                else {
                    swprintf_s(buf, L" E:%d A:%d W:%d KILL:%d [%d/100]",
                        aliveE, aliveA, waveCount, totalKills, currentLevel + 1);
                }
                drawText(screenBuf, SCREEN_W / 2 - 18, 0, buf, 12, 0);

                swprintf_s(buf, L" [%s]", diffCfg->name);
                drawText(screenBuf, 1, 0, buf, 11, 0);

                swprintf_s(buf, L" 1:%d 2:%d 3:%d 4:%d",
                    player.magAmmo[0], player.magAmmo[1],
                    player.magAmmo[2], player.magAmmo[3]);
                drawText(screenBuf, 1, 1, buf, 14, 0);

                if (!endlessMode) {
                    swprintf_s(buf, L" %s", MISSIONS[currentLevel].title);
                    drawText(screenBuf, 1, 2, buf, 11, 0);
                }

                if (waveTimer < 90 && waveCount > 1) {
                    if (endlessMode) {
                        swprintf_s(buf, L" >>> WAVE %d <<< ", endlessWave);
                    }
                    else {
                        swprintf_s(buf, L" >>> WAVE %d <<< ", waveCount);
                    }
                    if ((waveTimer / 8) % 2)
                        drawText(screenBuf, SCREEN_W / 2 - 8, 2, buf, 14, 0);
                }
                if (statusTimer > 0) {
                    statusTimer--;
                    int len = (int)netStatus.size();
                    drawText(screenBuf, (SCREEN_W - len) / 2, SCREEN_H - 3,
                        netStatus, anyTextureLoaded ? 10 : 14, 0);
                }
                drawText(screenBuf, SCREEN_W - 14, SCREEN_H - 1, L" [ESC] \u8BBE\u7F6E ", 8, 0);
            }
        }

        if (gameState == GS_PLAYING) {
            autoSaveTimer++;
            if (autoSaveTimer >= 150) { autoSaveTimer = 0; saveGame(); }
        }

        WriteConsoleOutputW(hOut, &screenBuf[0][0], writeSize, writeCoord, &writeRegion);

        auto frameEnd = std::chrono::steady_clock::now();
        int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(
            frameEnd - frameStart).count();
        if (elapsed < TARGET_MS) Sleep(TARGET_MS - elapsed);
    }

    if (gameState == GS_PLAYING || gameState == GS_PAUSED) saveGame();

    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
    return 0;
}