#include <jni.h>
#include <dlfcn.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <sys/mman.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"
#include "GameData.h"

#define LOG_TAG "SO2Mod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ==================== ARM64 Hook Helpers ====================
static inline uint32_t arm64_movz(int reg, uint16_t imm, int shift) {
    return 0xD2800000 | ((shift & 3) << 21) | ((uint32_t)imm << 5) | (reg & 0x1F);
}
static inline uint32_t arm64_movk(int reg, uint16_t imm, int shift) {
    return 0xF2800000 | ((shift & 3) << 21) | ((uint32_t)imm << 5) | (reg & 0x1F);
}
static inline uint32_t arm64_br(int reg) {
    return 0xD61F0000 | ((reg & 0x1F) << 5);
}
static void arm64_patch_jump(uint32_t* target, uint64_t dest) {
    target[0] = arm64_movz(16, dest & 0xFFFF, 0);
    target[1] = arm64_movk(16, (dest >> 16) & 0xFFFF, 1);
    target[2] = arm64_movk(16, (dest >> 32) & 0xFFFF, 2);
    target[3] = arm64_br(16);
}
static bool make_writable(void* addr, size_t len) {
    long ps = sysconf(_SC_PAGESIZE);
    void* base = (void*)((uintptr_t)addr & ~(ps - 1));
    return mprotect(base, len + ((uintptr_t)addr - (uintptr_t)base), PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}
static void* alloc_exec_mem(size_t size) {
    return mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

// ==================== Feature Manager ====================
struct Feature {
    const char* name;
    bool enabled;
    float value;
    float minVal, maxVal;
    int type;
    const char** items;
    int itemCount;
    int selectedItem;
};

class FeatureManager {
public:
    static FeatureManager& getInstance() { static FeatureManager i; return i; }
    std::vector<Feature> features;

    void init() {
        static const char* bone[] = {"Head","Neck","Chest","Stomach"};
        static const char* chams[] = {"Normal","Flat","Glow","Wireframe"};
        static const char* cross[] = {"Dot","Cross","Circle","Triangle"};

        addFeature("Box ESP");        addFeature("Snaplines ESP");
        addFeature("Health Bar");     addFeature("Name ESP");
        addFeature("Distance ESP");   addFeature("Weapon ESP");
        addFeature("Head Dot");
        addFeature("Aimbot");
        addSlider("Aimbot FOV", 10, 300, 90);
        addSlider("Aimbot Smooth", 1, 20, 5);
        addSpinner("Aimbot Bone", bone, 4);
        addFeature("Silent Aim");     addFeature("Predict Movement");
        addFeature("Triggerbot");
        addSlider("Trigger Delay", 0, 500, 50);
        addFeature("No Recoil");      addFeature("No Spread");
        addFeature("Infinite Ammo");  addFeature("Rapid Fire");
        addFeature("Magic Bullet");   addFeature("One Hit Kill");
        addFeature("Wallhack");       addFeature("Chams");
        addSpinner("Chams Style", chams, 4);
        addFeature("Glow ESP");       addFeature("Crosshair");
        addSpinner("Crosshair Style", cross, 4);
        addFeature("No Fog");         addFeature("Night Mode");
        addFeature("Colorful Sky");
        addFeature("Speed Hack");
        addSlider("Speed Multiplier", 1, 5, 1);
        addFeature("Bunny Hop");      addFeature("No Clip");
        addFeature("Fly Hack");
        addFeature("Anti Spectate");  addFeature("Streamer Mode");
        addFeature("Fake Lag");       addFeature("Fake Name");
    }
    void addFeature(const char* n) { features.push_back({n,false,0,0,1,0,nullptr,0,0}); }
    void addSlider(const char* n, float mn, float mx, float d) { features.push_back({n,false,d,mn,mx,1,nullptr,0,0}); }
    void addSpinner(const char* n, const char** items, int c) { features.push_back({n,false,0,0,1,2,items,c,0}); }
    Feature& get(const char* n) { for(auto& f:features) if(strcmp(f.name,n)==0) return f; static Feature e; return e; }
};

// ==================== Globals ====================
float g_ScreenW = 0, g_ScreenH = 0;
static std::atomic<bool> g_ShowMenu{false};
static std::atomic<bool> g_ImGuiInited{false};
static int g_SelectedTab = 0;

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t o_eglSwapBuffers = nullptr;

struct TouchState {
    std::mutex mtx;
    float x=0, y=0;
    std::atomic<bool> down{false}, downFrame{false}, upFrame{false};
};
static TouchState g_Touch;

static float g_FabX=60, g_FabY=300;
static const float FAB_SIZE=50;
static bool g_FabDragging=false;
static float g_FabOffX, g_FabOffY, g_FabStartX, g_FabStartY;
static bool g_FabMoved=false;

// ==================== Floating Button ====================
void DrawFab(ImDrawList* dl) {
    ImVec2 c(g_FabX+FAB_SIZE*0.5f, g_FabY+FAB_SIZE*0.5f);
    float r=FAB_SIZE*0.5f;
    for(int i=3;i>0;i--){float gr=r+i*5;dl->AddCircleFilled(c,gr,IM_COL32(88,166,255,(int)(30.f/i)),32);}
    dl->AddCircleFilled(ImVec2(c.x+2,c.y+3),r+1,IM_COL32(0,0,0,60),32);
    dl->AddCircleFilled(c,r,g_ShowMenu?IM_COL32(248,81,73,230):IM_COL32(31,111,235,230),32);
    dl->AddCircle(c,r,g_ShowMenu?IM_COL32(255,123,114,255):IM_COL32(100,180,255,255),32,2.5f);
    const char* t=g_ShowMenu?"X":"S2";
    ImVec2 ts=ImGui::CalcTextSize(t);
    dl->AddText(ImVec2(c.x-ts.x*0.5f,c.y-ts.y*0.5f+1),IM_COL32(255,255,255,255),t);
}

// ==================== ESP ====================
void DrawESP(ImDrawList* dl) {
    auto& fm=FeatureManager::getInstance();
    bool any=fm.get("Box ESP").enabled||fm.get("Snaplines ESP").enabled||
             fm.get("Health Bar").enabled||fm.get("Name ESP").enabled||
             fm.get("Distance ESP").enabled||fm.get("Head Dot").enabled;
    if(!any) return;

    auto& gd=GameData::getInstance();
    for(auto& p:gd.players) {
        if(!p.valid||!p.onScreen) continue;
        if(p.screenX<=0||p.screenY<=0||p.screenX>=g_ScreenW||p.screenY>=g_ScreenH) continue;
        if(p.isLocal) continue;

        ImU32 col=(p.team==1)?IM_COL32(0,120,255,255):IM_COL32(255,60,60,255);
        ImU32 shadow=IM_COL32(0,0,0,200);

        float halfW=p.boxW*0.5f;
        float topY=p.headScreenY;
        float botY=p.screenY;

        if(fm.get("Box ESP").enabled) {
            dl->AddRect(ImVec2(p.screenX-halfW-1,topY-1),ImVec2(p.screenX+halfW+1,botY+1),IM_COL32(0,0,0,150),0,0,1.5f);
            dl->AddRect(ImVec2(p.screenX-halfW,topY),ImVec2(p.screenX+halfW,botY),col,0,0,1.f);
        }
        if(fm.get("Head Dot").enabled) {
            dl->AddCircleFilled(ImVec2(p.headScreenX,p.headScreenY),4,col,12);
        }
        if(fm.get("Snaplines ESP").enabled) {
            dl->AddLine(ImVec2(g_ScreenW*0.5f,g_ScreenH),ImVec2(p.screenX,p.screenY),IM_COL32(255,255,255,60),1.f);
        }
        if(fm.get("Health Bar").enabled) {
            float hp=(float)p.health/(float)p.maxHealth;
            if(hp<0)hp=0;if(hp>1)hp=1;
            float x=p.screenX-halfW-6,y=p.screenY,barW=3,barH=p.boxH;
            dl->AddRectFilled(ImVec2(x-1,y-1),ImVec2(x+barW+1,y+barH+1),IM_COL32(0,0,0,150),1);
            dl->AddRectFilled(ImVec2(x,y+barH*(1-hp)),ImVec2(x+barW,y+barH),IM_COL32((int)(255*(1-hp)),(int)(255*hp),0,255),1);
        }
        if(fm.get("Name ESP").enabled) {
            char nm[32];snprintf(nm,sizeof(nm),"P%d",p.actorId);
            ImVec2 ts=ImGui::CalcTextSize(nm);
            dl->AddText(ImVec2(p.screenX-ts.x*0.5f,topY-18),shadow,nm);
            dl->AddText(ImVec2(p.screenX-ts.x*0.5f+1,topY-17),IM_COL32(255,255,255,220),nm);
        }
        if(fm.get("Distance ESP").enabled&&p.distance>0) {
            char d[32];snprintf(d,sizeof(d),"%.0fm",p.distance);
            ImVec2 ts=ImGui::CalcTextSize(d);
            dl->AddText(ImVec2(p.screenX-ts.x*0.5f,botY+4),IM_COL32(255,255,255,180),d);
        }
    }
}

// ==================== Crosshair ====================
void DrawCrosshair(ImDrawList* dl) {
    auto& fm=FeatureManager::getInstance();
    if(!fm.get("Crosshair").enabled) return;
    float cx=g_ScreenW*0.5f,cy=g_ScreenH*0.5f,sz=12,gap=4;
    ImU32 col=IM_COL32(255,255,255,200);
    int s=fm.get("Crosshair").selectedItem;
    if(s==0) dl->AddCircleFilled(ImVec2(cx,cy),3,col,12);
    else if(s==1){
        dl->AddLine(ImVec2(cx-sz,cy),ImVec2(cx-gap,cy),col,2);
        dl->AddLine(ImVec2(cx+gap,cy),ImVec2(cx+sz,cy),col,2);
        dl->AddLine(ImVec2(cx,cy-sz),ImVec2(cx,cy-gap),col,2);
        dl->AddLine(ImVec2(cx,cy+gap),ImVec2(cx,cy+sz),col,2);
    } else if(s==2) dl->AddCircle(ImVec2(cx,cy),sz,col,24,2);
    else dl->AddTriangle(ImVec2(cx,cy-sz),ImVec2(cx-sz,cy+sz),ImVec2(cx+sz,cy+sz),col,2);
}

// ==================== Menu ====================
static const char* tabNames[]={"ESP","Aimbot","Trigger","Player","World","Movement","Misc"};

void DrawMenu() {
    if(!g_ShowMenu) return;
    ImGuiIO& io=ImGui::GetIO();
    float w=380,h=440;
    ImVec2 pos((io.DisplaySize.x-w)*0.5f,(io.DisplaySize.y-h)*0.5f);
    ImGui::SetNextWindowPos(pos,ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,12);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8,5));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,6);
    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding,4);
    ImGui::PushStyleVar(ImGuiStyleVar_TabRounding,6);

    ImVec4 accent(0.122f,0.424f,0.929f,1),accentDim(0.122f,0.424f,0.929f,0.3f);
    ImVec4 text(0.851f,0.882f,0.918f,1),dim(0.545f,0.580f,0.616f,1);

    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.063f,0.067f,0.082f,0.96f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0.188f,0.212f,0.243f,1));
    ImGui::PushStyleColor(ImGuiCol_Text,text);
    ImGui::PushStyleColor(ImGuiCol_Tab,ImVec4(0.086f,0.090f,0.106f,1));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,accentDim);
    ImGui::PushStyleColor(ImGuiCol_TabActive,accent);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0.102f,0.106f,0.122f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(0.137f,0.145f,0.165f,1));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive,accentDim);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,ImVec4(0.2f,0.5f,1,1));
    ImGui::PushStyleColor(ImGuiCol_CheckMark,accent);
    ImGui::PushStyleColor(ImGuiCol_Header,accentDim);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.122f,0.424f,0.929f,0.5f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg,ImVec4(0.08f,0.08f,0.10f,0.96f));
    ImGui::PushStyleColor(ImGuiCol_Button,accentDim);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.122f,0.424f,0.929f,0.8f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,ImVec4(0.188f,0.212f,0.243f,1));

    ImGui::Begin("##m",nullptr,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize|
                                ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl=ImGui::GetWindowDrawList();
    ImVec2 hm=ImGui::GetCursorScreenPos();
    dl->AddRectFilled(hm,ImVec2(hm.x+ImGui::GetContentRegionAvail().x,hm.y+40),IM_COL32(22,27,34,255),12,ImDrawFlags_RoundCornersTop);

    ImGui::SetCursorPosX(14);ImGui::SetCursorPosY(10);
    ImGui::TextColored(accent,"S2");ImGui::SameLine();
    ImGui::TextColored(text,"Mod Menu");ImGui::SameLine(ImGui::GetContentRegionAvail().x-30);
    ImGui::TextColored(dim,"v4.0");
    ImGui::SetCursorPosY(46);

    ImGui::SetCursorPosX(8);
    if(ImGui::BeginTabBar("##t",ImGuiTabBarFlags_FittingPolicyScroll)){
        for(int i=0;i<7;i++) if(ImGui::BeginTabItem(tabNames[i])){g_SelectedTab=i;ImGui::EndTabItem();}
        ImGui::EndTabBar();
    }
    ImGui::Separator();
    ImGui::BeginChild("##c",ImVec2(0,0),false);
    ImGui::SetCursorPosX(12);

    FeatureManager& fm=FeatureManager::getInstance();
    auto Toggle=[&](int i){
        bool en=fm.features[i].enabled;
        ImGui::PushID(i);
        ImVec2 p=ImGui::GetCursorScreenPos();
        if(ImGui::InvisibleButton("##t",ImVec2(36,18)))fm.features[i].enabled=!en;
        en=fm.features[i].enabled;
        dl->AddRectFilled(p,ImVec2(p.x+36,p.y+18),en?IM_COL32(31,111,235,255):IM_COL32(50,50,55,255),9);
        dl->AddCircleFilled(ImVec2(en?p.x+26:p.x+10,p.y+9),6,IM_COL32(255,255,255,240));
        ImGui::SameLine(44);
        ImGui::TextColored(en?text:dim,"%s",fm.features[i].name);
        ImGui::PopID();
    };
    auto Slider=[&](int i){
        ImGui::PushID(i+200);ImGui::TextColored(dim,"%s",fm.features[i].name);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        char buf[32];snprintf(buf,sizeof(buf),"%.1f",fm.features[i].value);
        ImGui::SliderFloat("##s",&fm.features[i].value,fm.features[i].minVal,fm.features[i].maxVal,buf);
        ImGui::PopID();
    };
    auto Combo=[&](int i){
        ImGui::PushID(i+300);ImGui::TextColored(dim,"%s",fm.features[i].name);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if(ImGui::BeginCombo("##x",fm.features[i].items[fm.features[i].selectedItem])){
            for(int j=0;j<fm.features[i].itemCount;j++)
                if(ImGui::Selectable(fm.features[i].items[j],fm.features[i].selectedItem==j))fm.features[i].selectedItem=j;
            ImGui::EndCombo();
        }
        ImGui::PopID();
    };

    switch(g_SelectedTab){
        case 0:for(int i=0;i<7;i++)Toggle(i);break;
        case 1:Toggle(7);if(fm.features[7].enabled){Slider(8);Slider(9);Combo(10);}
            Toggle(11);Toggle(12);break;
        case 2:Toggle(13);if(fm.features[13].enabled)Slider(14);break;
        case 3:for(int i=15;i<=20;i++)Toggle(i);break;
        case 4:Toggle(21);Toggle(22);if(fm.features[22].enabled)Combo(23);
            Toggle(24);Toggle(25);if(fm.features[25].enabled)Combo(26);
            Toggle(27);Toggle(28);Toggle(29);break;
        case 5:Toggle(30);if(fm.features[30].enabled)Slider(31);
            Toggle(32);Toggle(33);Toggle(34);break;
        case 6:for(int i=35;i<=38;i++)Toggle(i);break;
    }
    ImGui::EndChild();ImGui::End();
    ImGui::PopStyleColor(20);ImGui::PopStyleVar(6);
}

// ==================== Touch ====================
static void process_touch(float x,float y,bool down,bool df,bool uf) {
    ImGuiIO& io=ImGui::GetIO();
    float dx=x-(g_FabX+FAB_SIZE*0.5f),dy=y-(g_FabY+FAB_SIZE*0.5f);
    if(df&&sqrtf(dx*dx+dy*dy)<FAB_SIZE*0.5f+15){
        g_FabDragging=true;g_FabMoved=false;g_FabOffX=x-g_FabX;g_FabOffY=y-g_FabY;
        g_FabStartX=x;g_FabStartY=y;return;
    }
    if(g_FabDragging){
        if(down){if(fabsf(x-g_FabStartX)>8||fabsf(y-g_FabStartY)>8)g_FabMoved=true;g_FabX=x-g_FabOffX;g_FabY=y-g_FabOffY;}
        if(uf){g_FabDragging=false;if(!g_FabMoved)g_ShowMenu=!g_ShowMenu;}
        return;
    }
    if(!g_ShowMenu) return;
    io.MousePos=ImVec2(x,y);
    if(df)io.MouseDown[0]=true;
    if(uf)io.MouseDown[0]=false;
}

// ==================== Game Update ====================
static void gameUpdate() {
    auto& gd=GameData::getInstance();
    if(!gd.initialized) return;

    gd.updateLocalPlayer();
    gd.updateCamera();
    if(gd.mainCamera) gd.readCameraMatrices();
    gd.updatePlayers();
}

// ==================== Aimbot ====================
static void aimbotUpdate() {
    auto& fm=FeatureManager::getInstance();
    if(!fm.get("Aimbot").enabled) return;

    auto& gd=GameData::getInstance();
    if(!gd.localController) return;

    float fov=fm.get("Aimbot FOV").value;
    float smooth=fm.get("Aimbot Smooth").value;
    if(smooth<1) smooth=1;

    PlayerData* best=nullptr;
    float bestDist=fov;

    int localTeam=getInt(gd.localController, gd.pc_team);

    for(auto& p:gd.players) {
        if(!p.valid||p.isLocal||!p.isAlive||p.team==0) continue;
        if(p.team==localTeam) continue;
        if(!p.onScreen) continue;

        float dx=p.screenX-g_ScreenW*0.5f;
        float dy=p.screenY-g_ScreenH*0.5f;
        float dist=sqrtf(dx*dx+dy*dy);

        if(dist<bestDist) { bestDist=dist; best=&p; }
    }

    if(best) gd.aimAtTarget(best->headX, best->headY, best->headZ, smooth);
}

// ==================== Triggerbot ====================
static void triggerbotUpdate() {
    auto& fm=FeatureManager::getInstance();
    if(!fm.get("Triggerbot").enabled) return;
    // TODO: Detect crosshair over enemy and simulate fire
}

// ==================== eglSwapBuffers Hook ====================
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if(!g_ImGuiInited) {
        EGLint w,h;
        eglQuerySurface(dpy,surf,EGL_WIDTH,&w);
        eglQuerySurface(dpy,surf,EGL_HEIGHT,&h);
        g_ScreenW=(float)w;g_ScreenH=(float)h;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io=ImGui::GetIO();
        io.DisplaySize=ImVec2(g_ScreenW,g_ScreenH);
        io.IniFilename=nullptr;
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();
        ImGuiStyle& s=ImGui::GetStyle();
        s.WindowRounding=12;s.FrameRounding=6;s.GrabRounding=4;s.TabRounding=6;
        s.ScaleAllSizes(1.5f);

        ImGui_ImplAndroid_Init(nullptr);
        ImGui_ImplOpenGL3_Init("#version 300 es");
        g_ImGuiInited=true;
        LOGI("ImGui init: %dx%d",w,h);

        std::thread([](){
            usleep(2000000);
            GameData::getInstance().init();
        }).detach();
    }

    // Save GL state
    GLint lp,lt,la,lv;glGetIntegerv(GL_CURRENT_PROGRAM,&lp);glGetIntegerv(GL_TEXTURE_BINDING_2D,&lt);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&la);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&lv);
    GLint lvp[4],lsc[4];glGetIntegerv(GL_VIEWPORT,lvp);glGetIntegerv(GL_SCISSOR_BOX,lsc);
    GLboolean lc=glIsEnabled(GL_CULL_FACE),ld=glIsEnabled(GL_DEPTH_TEST),
               lb=glIsEnabled(GL_BLEND),lst=glIsEnabled(GL_SCISSOR_TEST);

    gameUpdate();
    aimbotUpdate();
    triggerbotUpdate();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    {std::lock_guard<std::mutex> lk(g_Touch.mtx);
     bool df=g_Touch.downFrame.exchange(false),uf=g_Touch.upFrame.exchange(false);
     process_touch(g_Touch.x,g_Touch.y,g_Touch.down,df,uf);}

    ImDrawList* bg=ImGui::GetBackgroundDrawList();
    DrawFab(bg);DrawESP(bg);DrawCrosshair(bg);DrawMenu();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Restore GL
    glUseProgram(lp);glBindTexture(GL_TEXTURE_2D,lt);glBindBuffer(GL_ARRAY_BUFFER,la);glBindVertexArray(lv);
    glViewport(lvp[0],lvp[1],lvp[2],lvp[3]);glScissor(lsc[0],lsc[1],lsc[2],lsc[3]);
    if(lc)glEnable(GL_CULL_FACE);else glDisable(GL_CULL_FACE);
    if(ld)glEnable(GL_DEPTH_TEST);else glDisable(GL_DEPTH_TEST);
    if(lb)glEnable(GL_BLEND);else glDisable(GL_BLEND);
    if(lst)glEnable(GL_SCISSOR_TEST);else glDisable(GL_SCISSOR_TEST);

    return o_eglSwapBuffers(dpy,surf);
}

// ==================== Input Hook ====================
#include <android/input.h>
typedef void(*InputConsumer_t)(void*,void*,void*);
static InputConsumer_t o_InputConsumer=nullptr;

void hook_InputConsumer(void* thiz,void* a2,void* a3) {
    ((InputConsumer_t)o_InputConsumer)(thiz,a2,a3);
    if(a2){
        AInputEvent* ev=(AInputEvent*)a2;
        if(AInputEvent_getType(ev)==AINPUT_EVENT_TYPE_MOTION){
            int32_t act=AMotionEvent_getAction(ev);
            float x=AMotionEvent_getX(ev,0),y=AMotionEvent_getY(ev,0);
            std::lock_guard<std::mutex> lk(g_Touch.mtx);
            g_Touch.x=x;g_Touch.y=y;
            switch(act&AMOTION_EVENT_ACTION_MASK){
                case AMOTION_EVENT_ACTION_DOWN:g_Touch.down=true;g_Touch.downFrame=true;break;
                case AMOTION_EVENT_ACTION_MOVE:break;
                case AMOTION_EVENT_ACTION_UP:
                case AMOTION_EVENT_ACTION_CANCEL:g_Touch.down=false;g_Touch.upFrame=true;break;
            }
        }
    }
}

// ==================== Hook Setup ====================
static void initHooks() {
    for(int i=0;i<50;i++){if(dlopen("libEGL.so",RTLD_LAZY|RTLD_NOLOAD))break;usleep(100000);}

    void* hEGL=dlopen("libEGL.so",RTLD_LAZY|RTLD_NOLOAD);
    if(!hEGL){LOGE("No libEGL.so");return;}
    void* sym=dlsym(hEGL,"eglSwapBuffers");dlclose(hEGL);
    if(!sym){LOGE("No eglSwapBuffers");return;}
    LOGI("eglSwapBuffers: %p",sym);

    if(!make_writable(sym,16)){LOGE("mprotect failed");return;}
    void* tramp=alloc_exec_mem(4096);
    if(tramp==MAP_FAILED){LOGE("mmap failed");return;}
    memcpy(tramp,sym,16);
    arm64_patch_jump((uint32_t*)tramp+4,(uint64_t)((uint8_t*)sym+16));
    o_eglSwapBuffers=(eglSwapBuffers_t)tramp;
    arm64_patch_jump((uint32_t*)sym,(uint64_t)hook_eglSwapBuffers);
    __builtin___clear_cache((char*)sym,(char*)sym+16);
    LOGI("eglSwapBuffers hooked: %p -> %p",sym,hook_eglSwapBuffers);

    void* hInput=dlopen("libinput.so",RTLD_LAZY|RTLD_NOLOAD);
    if(hInput){
        void* iSym=dlsym(hInput,"_ZN7android13InputConsumer21initializeMotionEventEPNS_11MotionEventEPKNS_12InputMessageE");
        if(!iSym) iSym=dlsym(hInput,"_ZN7android13InputConsumer7consumeEPNS_26InputEventFactoryInterfaceEblPjPPNS_10InputEventE");
        dlclose(hInput);
        if(iSym&&make_writable(iSym,16)){
            void* t2=alloc_exec_mem(4096);
            if(t2!=MAP_FAILED){
                memcpy(t2,iSym,16);arm64_patch_jump((uint32_t*)t2+4,(uint64_t)((uint8_t*)iSym+16));
                o_InputConsumer=(InputConsumer_t)t2;
                arm64_patch_jump((uint32_t*)iSym,(uint64_t)hook_InputConsumer);
                __builtin___clear_cache((char*)iSym,(char*)iSym+16);
                LOGI("InputConsumer hooked");
            }
        }
    }
}

// ==================== JNI ====================
extern "C" {
void loadJNI(JavaVM* vm) {
    LOGI("loadJNI v4.0");
    FeatureManager::getInstance().init();
    std::thread([](){usleep(3000000);initHooks();}).detach();
}

JNIEXPORT void JNICALL Java_com_modmenu_loader_NativeLoader_toggleMenu(JNIEnv*,jclass){g_ShowMenu=!g_ShowMenu;}
JNIEXPORT jboolean JNICALL Java_com_modmenu_loader_NativeLoader_isFeatureEnabled(JNIEnv*,jclass,jint i){
    auto& f=FeatureManager::getInstance().features;
    return(i>=0&&i<(int)f.size()&&f[i].enabled)?JNI_TRUE:JNI_FALSE;
}
JNIEXPORT jfloat JNICALL Java_com_modmenu_loader_NativeLoader_getFeatureValue(JNIEnv*,jclass,jint i){
    auto& f=FeatureManager::getInstance().features;
    return(i>=0&&i<(int)f.size())?f[i].value:0;
}
}
