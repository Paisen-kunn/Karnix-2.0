/*
 * KarnixOS – Spine 3D Viewer
 * OpenGL 2.1 + GLFW3 + libpq
 *
 * Reads the same PostgreSQL credentials from
 *   ../spine_analytics_dashboard/backend/.env
 * and shows every patient as a fully animated 3D object.
 *
 * Build:  see Makefile   ( make && ./spine3d )
 *
 * Controls:
 *   Click a pillar  – zoom into detailed 3D spine model
 *   Esc / click [←] – return to overview
 *   Mouse drag      – rotate camera (overview)
 *   Scroll          – zoom in/out (overview)
 */

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <libpq-fe.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

/* ═══════════════════════════════════════════════
   Data
═══════════════════════════════════════════════ */
struct Patient {
    char id[20];
    int  age;
    char sex[4];
    char date[16];
    float risk_score;   /* 0-100 */
    float abnorm_pct;   /* 0-100, higher = more diseased */
    char  classification[32];
    char  risk_cat[16];    /* HIGH / MODERATE / LOW */
    float cobb_angle;
    float bone_density;
    int   frac_grade;
    int   disc_grade;
    /* rendering state */
    float  r,g,b;          /* body color */
    float  bar_target;     /* target bar height (0-5) */
    float  bar_cur;        /* animated current height */
    float  pulse_phase;    /* per-patient phase offset */
    float  rise_delay;     /* staggered rise delay (seconds) */
    int    grid_row, grid_col;
};

static std::vector<Patient> g_patients;
static int  g_selected   = -1;   /* index of selected patient */
static bool g_detail_view= false;

/* ═══════════════════════════════════════════════
   Camera & animation globals
═══════════════════════════════════════════════ */
static float g_cam_yaw   = 20.0f;   /* orbit angle */
static float g_cam_pitch = 25.0f;
static float g_cam_dist  = 30.0f;

static float g_detail_spin  = 0.0f; /* spine rotation in detail */

static float g_time = 0.0f;
static float g_prev_time = 0.0f;

static GLFWwindow *g_win = nullptr;
static int g_win_w = 1400, g_win_h = 860;

/* drag */
static bool   g_drag = false;
static double g_drag_last_x, g_drag_last_y;

/* tooltip / hover */
static int g_hover = -1;
static double g_mouse_x, g_mouse_y;
struct PillarScreen { float sx, sy; };
static std::vector<PillarScreen> g_pillar_screen;

/* ═══════════════════════════════════════════════
   Env loader
═══════════════════════════════════════════════ */
static std::string env_get(const char *path, const char *key) {
    FILE *f = fopen(path, "r");
    if (!f) return "";
    char line[256];
    std::string sk(key);
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        std::string k(line, eq);
        if (k == sk) {
            std::string v(eq+1);
            while (!v.empty() && (v.back()=='\n'||v.back()=='\r'||v.back()==' '))
                v.pop_back();
            fclose(f); return v;
        }
    }
    fclose(f); return "";
}

/* ═══════════════════════════════════════════════
   PostgreSQL loader
═══════════════════════════════════════════════ */
static const char *ENV_PATH =
    "../spine_analytics_dashboard/backend/.env";

/* fallback hardcoded sample (when DB is unavailable) */
static void load_fallback() {
    static const struct { const char*id; int age; const char*sex;
        float rs; float ap; const char*cl; const char*cat; float cobb; float bone; int fg; int dg;
    } fb[] = {
        {"PT-001",45,"M", 57.7f,99,"Abnormal","HIGH",18,-1.2f,1,2},
        {"PT-002",32,"F",100.0f, 0,"Normal","LOW",4,0.5f,0,0},
        {"PT-003",58,"M", 42.1f,87,"Abnormal","HIGH",24,-2.1f,2,3},
        {"PT-004",29,"F", 93.4f, 6,"Normal","LOW",3,1.1f,0,0},
        {"PT-005",67,"M", 12.3f,76,"DiskHernia","HIGH",8,-2.8f,1,4},
        {"PT-006",41,"F", 89.0f,11,"Normal","LOW",5,0.2f,0,1},
        {"PT-007",53,"M", 31.0f,91,"Spondylolisthesis","HIGH",12,-1.8f,2,3},
        {"PT-008",36,"F", 98.5f, 1,"Normal","LOW",2,0.8f,0,0},
        {"PT-009",72,"M",  8.9f,96,"Abnormal","HIGH",31,-3.1f,3,4},
        {"PT-010",44,"F", 75.6f,24,"DiskHernia","MODERATE",9,-0.5f,1,2},
        {"PT-011",38,"M", 92.1f, 7,"Normal","LOW",4,0.6f,0,0},
        {"PT-012",61,"F", 20.4f,84,"Spondylolisthesis","HIGH",19,-2.4f,2,3},
        {"PT-013",27,"M", 87.6f,12,"Normal","LOW",3,1.2f,0,0},
        {"PT-014",55,"F", 44.8f,69,"DiskHernia","MODERATE",11,-1.0f,1,3},
        {"PT-015",48,"M", 61.1f,38,"DiskHernia","MODERATE",7,-0.8f,1,2},
        {"PT-016",33,"F", 99.9f, 0,"Normal","LOW",2,1.0f,0,0},
        {"PT-017",76,"M",  5.5f,98,"Abnormal","HIGH",36,-3.5f,3,4},
        {"PT-018",42,"F", 78.0f,21,"Normal","LOW",5,0.3f,0,1},
        {"PT-019",59,"M", 31.5f,88,"Spondylolisthesis","HIGH",14,-1.5f,2,3},
        {"PT-020",31,"F", 94.5f, 5,"Normal","LOW",3,0.9f,0,0},
    };
    g_patients.clear();
    for (auto &f : fb) {
        Patient p{}; snprintf(p.id,sizeof(p.id),"%s",f.id);
        p.age=f.age; snprintf(p.sex,sizeof(p.sex),"%s",f.sex);
        p.risk_score=f.rs; p.abnorm_pct=f.ap;
        snprintf(p.classification,sizeof(p.classification),"%s",f.cl);
        snprintf(p.risk_cat,sizeof(p.risk_cat),"%s",f.cat);
        p.cobb_angle=f.cobb; p.bone_density=f.bone;
        p.frac_grade=f.fg; p.disc_grade=f.dg;
        g_patients.push_back(p);
    }
}

static void load_from_db() {
    std::string host = env_get(ENV_PATH,"DB_HOST");
    std::string port = env_get(ENV_PATH,"DB_PORT");
    std::string user = env_get(ENV_PATH,"DB_USER");
    std::string pass = env_get(ENV_PATH,"DB_PASSWORD");
    std::string name = env_get(ENV_PATH,"DB_NAME");
    if (host.empty()) host="localhost";
    if (port.empty()) port="5432";
    if (name.empty()) name="postgres";

    char connstr[512];
    snprintf(connstr,sizeof(connstr),
        "host=%s port=%s user=%s password=%s dbname=%s connect_timeout=4",
        host.c_str(),port.c_str(),user.c_str(),pass.c_str(),name.c_str());

    PGconn *conn = PQconnectdb(connstr);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr,"[spine3d] DB connect failed: %s  –  using fallback data\n",
                PQerrorMessage(conn));
        PQfinish(conn); load_fallback(); return;
    }

    const char *sql =
        "SELECT patient_id, COALESCE(age,0), UPPER(COALESCE(sex,'U')),"
        "  TO_CHAR(scan_date,'YYYY-MM-DD'),"
        "  COALESCE(risk_score,50), COALESCE(abnormal_prob,50),"
        "  COALESCE(dl_classification,'Unknown'),"
        "  COALESCE(risk_category,'MODERATE'),"
        "  COALESCE(cobb_angle,0), COALESCE(bone_density_tscore,0),"
        "  COALESCE(compression_fracture_grade,0),"
        "  COALESCE(disc_degeneration_grade,0)"
        " FROM patients ORDER BY scan_date DESC LIMIT 80";

    PGresult *res = PQexec(conn, sql);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr,"[spine3d] Query failed: %s  –  using fallback\n",
                PQerrorMessage(conn));
        PQclear(res); PQfinish(conn); load_fallback(); return;
    }

    g_patients.clear();
    int rows = PQntuples(res);
    fprintf(stdout,"[spine3d] Loaded %d patients from PostgreSQL\n", rows);
    for (int i=0; i<rows; i++) {
        Patient p{};
        snprintf(p.id,sizeof(p.id),"%s",PQgetvalue(res,i,0));
        p.age      = atoi(PQgetvalue(res,i,1));
        snprintf(p.sex,sizeof(p.sex),"%s",PQgetvalue(res,i,2));
        snprintf(p.date,sizeof(p.date),"%s",PQgetvalue(res,i,3));
        p.risk_score  = atof(PQgetvalue(res,i,4));
        p.abnorm_pct  = atof(PQgetvalue(res,i,5));
        snprintf(p.classification,sizeof(p.classification),"%s",PQgetvalue(res,i,6));
        snprintf(p.risk_cat,sizeof(p.risk_cat),"%s",PQgetvalue(res,i,7));
        p.cobb_angle   = atof(PQgetvalue(res,i,8));
        p.bone_density = atof(PQgetvalue(res,i,9));
        p.frac_grade   = atoi(PQgetvalue(res,i,10));
        p.disc_grade   = atoi(PQgetvalue(res,i,11));
        g_patients.push_back(p);
    }
    PQclear(res); PQfinish(conn);
    if (g_patients.empty()) load_fallback();
}

/* ═══════════════════════════════════════════════
   Patient color + bar height setup
═══════════════════════════════════════════════ */
static void setup_patient_visuals() {
    int cols = 8;
    for (int i=0; i<(int)g_patients.size(); i++) {
        Patient &p = g_patients[i];
        /* colour by risk category */
        if (strcmp(p.risk_cat,"HIGH")==0)         { p.r=1.0f; p.g=0.22f; p.b=0.18f; }
        else if (strcmp(p.risk_cat,"MODERATE")==0){ p.r=1.0f; p.g=0.78f; p.b=0.1f;  }
        else                                        { p.r=0.2f; p.g=0.95f; p.b=0.35f; }

        /* bar height: abnorm_pct mapped 0-5 units */
        p.bar_target = std::max(0.3f, p.abnorm_pct / 100.0f * 5.0f);
        p.bar_cur    = 0.0f;   /* starts at ground */
        p.pulse_phase = (float)i * 0.61803f * 6.2832f; /* golden-ratio phase spread */
        p.rise_delay  = (float)i * 0.04f;

        p.grid_col = i % cols;
        p.grid_row = i / cols;
    }
}

/* ═══════════════════════════════════════════════
   Simple math helpers
═══════════════════════════════════════════════ */
static inline float clamp01(float v) { return v<0?0:v>1?1:v; }

/* spring ease-out: t in [0,1] */
static float spring(float t) {
    t = clamp01(t);
    return 1.0f - expf(-6.0f*t)*cosf(8.0f*t);
}

/* ═══════════════════════════════════════════════
   OpenGL drawing helpers
═══════════════════════════════════════════════ */
static void set_material(float r, float g, float b, float shininess=32.0f) {
    float diff[4]  = {r,g,b,1};
    float spec[4]  = {0.7f,0.7f,0.7f,1};
    float ambi[4]  = {r*0.25f,g*0.25f,b*0.25f,1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ambi);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

/* Solid box centred at origin; w×h×d */
static void draw_box(float w, float h, float d) {
    float x=w*0.5f, y=h*0.5f, z=d*0.5f;
    glBegin(GL_QUADS);
    /* top */
    glNormal3f(0,1,0); glVertex3f(-x,y,-z); glVertex3f(x,y,-z); glVertex3f(x,y,z); glVertex3f(-x,y,z);
    /* bottom */
    glNormal3f(0,-1,0); glVertex3f(-x,-y,z); glVertex3f(x,-y,z); glVertex3f(x,-y,-z); glVertex3f(-x,-y,-z);
    /* front */
    glNormal3f(0,0,1); glVertex3f(-x,-y,z); glVertex3f(x,-y,z); glVertex3f(x,y,z); glVertex3f(-x,y,z);
    /* back */
    glNormal3f(0,0,-1); glVertex3f(x,-y,-z); glVertex3f(-x,-y,-z); glVertex3f(-x,y,-z); glVertex3f(x,y,-z);
    /* left */
    glNormal3f(-1,0,0); glVertex3f(-x,-y,-z); glVertex3f(-x,-y,z); glVertex3f(-x,y,z); glVertex3f(-x,y,-z);
    /* right */
    glNormal3f(1,0,0); glVertex3f(x,-y,z); glVertex3f(x,-y,-z); glVertex3f(x,y,-z); glVertex3f(x,y,z);
    glEnd();
}

/* Wire-frame box */
static void draw_box_wire(float w, float h, float d) {
    float x=w*0.5f, y=h*0.5f, z=d*0.5f;
    glBegin(GL_LINE_LOOP); glVertex3f(-x,-y,-z); glVertex3f(x,-y,-z); glVertex3f(x,y,-z); glVertex3f(-x,y,-z); glEnd();
    glBegin(GL_LINE_LOOP); glVertex3f(-x,-y,z);  glVertex3f(x,-y,z);  glVertex3f(x,y,z);  glVertex3f(-x,y,z);  glEnd();
    glBegin(GL_LINES);
    glVertex3f(-x,-y,-z); glVertex3f(-x,-y,z);
    glVertex3f( x,-y,-z); glVertex3f( x,-y,z);
    glVertex3f( x, y,-z); glVertex3f( x, y,z);
    glVertex3f(-x, y,-z); glVertex3f(-x, y,z);
    glEnd();
}

/* Cylinder approximation along Y axis, r=radius, h=height, segs=segments */
static void draw_cylinder(float r, float h, int segs=20) {
    float ha = h*0.5f;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i=0; i<=segs; i++) {
        float a = (float)i/(float)segs * 6.2832f;
        float nx=cosf(a), nz=sinf(a);
        glNormal3f(nx,0,nz);
        glVertex3f(nx*r,-ha,nz*r);
        glVertex3f(nx*r, ha,nz*r);
    }
    glEnd();
    /* caps */
    glBegin(GL_TRIANGLE_FAN); glNormal3f(0,1,0); glVertex3f(0,ha,0);
    for (int i=0;i<=segs;i++) { float a=(float)i/segs*6.2832f; glVertex3f(cosf(a)*r,ha,sinf(a)*r); } glEnd();
    glBegin(GL_TRIANGLE_FAN); glNormal3f(0,-1,0); glVertex3f(0,-ha,0);
    for (int i=segs;i>=0;i--) { float a=(float)i/segs*6.2832f; glVertex3f(cosf(a)*r,-ha,sinf(a)*r); } glEnd();
}

/* Glow ring on the floor under a pillar */
static void draw_glow_ring(float r, float g, float b, float alpha, float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    int segs = 32;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r,g,b,alpha); glVertex3f(0,0.01f,0);
    glColor4f(r,g,b,0);
    for (int i=0;i<=segs;i++) { float a=(float)i/segs*6.2832f; glVertex3f(cosf(a)*radius,0.01f,sinf(a)*radius); }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* Floor grid */
static void draw_grid(int nx, int nz, float spacing) {
    glDisable(GL_LIGHTING);
    glLineWidth(0.7f);
    glColor4f(0.25f,0.35f,0.55f,0.5f);
    glBegin(GL_LINES);
    float ox = -nx*spacing*0.5f, oz = -nz*spacing*0.5f;
    for (int i=0;i<=nx;i++) {
        float x=ox+i*spacing;
        glVertex3f(x,0,oz); glVertex3f(x,0,oz+nz*spacing);
    }
    for (int j=0;j<=nz;j++) {
        float z=oz+j*spacing;
        glVertex3f(ox,0,z); glVertex3f(ox+nx*spacing,0,z);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

/* 2D bitmap text helpers */
static void render_string_2d(float px, float py, const char *s, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glColor3f(r,g,b);
    glRasterPos2f(px,py);
    for (const char *c=s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*c);
    glEnable(GL_LIGHTING);
}
static void render_string_small(float px, float py, const char *s, float r, float g, float b) {
    glDisable(GL_LIGHTING);
    glColor3f(r,g,b);
    glRasterPos2f(px,py);
    for (const char *c=s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*c);
    glEnable(GL_LIGHTING);
}

/* Push a 2-D ortho transform (full-window pixels) */
static void push2d() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,g_win_w,0,g_win_h,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}
static void pop2d() {
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

/* filled rect in 2D pixel coords */
static void fill_rect_2d(float x, float y, float w, float h,
                          float r, float g, float b, float a=1.0f) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glBegin(GL_QUADS);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

/* ═══════════════════════════════════════════════
   OVERVIEW: pillar grid for all patients
═══════════════════════════════════════════════ */
static const float GRID_SPACING = 2.2f;
static const int   GRID_COLS    = 8;

static float pillar_world_x(const Patient &p) {
    return (p.grid_col - (GRID_COLS-1)/2.0f) * GRID_SPACING;
}
static float pillar_world_z(const Patient &p) {
    int total = (int)g_patients.size();
    int rows  = (total + GRID_COLS-1) / GRID_COLS;
    return (p.grid_row - (rows-1)/2.0f) * GRID_SPACING;
}

static void render_overview() {
    int n = (int)g_patients.size();
    int rows = (n + GRID_COLS-1) / GRID_COLS;

    /* Floor grid */
    draw_grid(GRID_COLS+1, rows+1, GRID_SPACING);

    /* ensure storage for screen-space coords */
    g_pillar_screen.resize(n);

    /* fetch matrices once – they're the same for every pillar this frame */
    double mvmat[16], prjmat[16]; int vp[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,  mvmat);
    glGetDoublev(GL_PROJECTION_MATRIX, prjmat);
    glGetIntegerv(GL_VIEWPORT, vp);

    for (int i=0; i<n; i++) {
        Patient &p = g_patients[i];
        float wx = pillar_world_x(p);
        float wz = pillar_world_z(p);

        /* ── pulse for HIGH risk ── */
        float pulse = 0.0f;
        if (strcmp(p.risk_cat,"HIGH")==0)
            pulse = sinf(g_time*4.0f + p.pulse_phase) * 0.12f;
        float bar_h = p.bar_cur + pulse;
        if (bar_h < 0.05f) bar_h = 0.05f;

        /* project pillar TOP to screen (wx, bar_h, wz) */
        {
            double sx, sy, sz;
            gluProject((double)wx, (double)bar_h, (double)wz,
                       mvmat, prjmat, vp, &sx, &sy, &sz);
            /* sy is bottom-left origin; GLFW is top-left – flip */
            g_pillar_screen[i] = { (float)sx, (float)(g_win_h - sy) };
        }

        bool sel = (i == g_selected);
        bool hov = (i == g_hover);

        glPushMatrix();
        glTranslatef(wx, bar_h*0.5f, wz);

        /* glow under pillar */
        glPushMatrix(); glTranslatef(0,-bar_h*0.5f,0);
        draw_glow_ring(p.r,p.g,p.b, sel?0.6f:hov?0.4f:0.25f,
                       sel?1.4f:hov?1.2f:0.9f);
        glPopMatrix();

        /* pillar body */
        float scale = sel ? 1.08f : hov ? 1.04f : 1.0f;
        glScalef(scale,1,scale);
        set_material(p.r, p.g, p.b, 48);
        draw_box(0.8f, bar_h, 0.8f);

        /* selection ring outline */
        if (sel) {
            glDisable(GL_LIGHTING);
            glLineWidth(2.5f);
            glColor3f(1,1,1);
            draw_box_wire(0.9f, bar_h+0.05f, 0.9f);
            glLineWidth(1.0f);
            glEnable(GL_LIGHTING);
        }

        /* TOP glow cap */
        set_material(std::min(p.r*1.6f,1.0f), std::min(p.g*1.6f,1.0f), std::min(p.b*1.6f,1.0f), 96);
        glTranslatef(0, bar_h*0.5f - 0.0f, 0);
        draw_box(0.82f, 0.08f, 0.82f);

        glPopMatrix();
    }
}

/* ═══════════════════════════════════════════════
   DETAIL: 3D spine model for one patient
═══════════════════════════════════════════════ */
struct SpineLevel {
    float y;         /* base Y */
    float w,h,d;     /* dimensions */
    bool  is_disc;
    const char *name;
    bool  affected;
    float shift_x, shift_z;
    bool  thoracic;  /* true = thoracic vertebra, has rib attachment */
};

static void build_spine_levels(const Patient &p, std::vector<SpineLevel> &levels) {
    levels.clear();
    bool disk = (strncmp(p.classification,"Disk",4)==0 ||
                 strncmp(p.classification,"Disc",4)==0 ||
                 strstr(p.classification,"Hern")!=nullptr);
    bool spond= strstr(p.classification,"Spondyl")!=nullptr;
    bool abn  = strcmp(p.classification,"Abnormal")==0;

    /* bottom-up: sacrum at y=0, lumbar then thoracic */
    struct Seg { float h; bool disc; const char* nm; bool aff; float sx; bool thor; };
    std::vector<Seg> segs = {
        /* sacrum + lumbar */
        {0.92f,false,"SACRUM", false, 0,    false},
        {0.13f,true, "D-L5S1",disk,  0,    false},
        {0.44f,false,"L5",    spond, spond?0.30f:0, false},
        {0.13f,true, "D-L45", disk,  0,    false},
        {0.44f,false,"L4",    abn||disk, 0, false},
        {0.13f,true, "D-L34", abn,   0,    false},
        {0.44f,false,"L3",    abn,   0,    false},
        {0.13f,true, "D-L23", false, 0,    false},
        {0.43f,false,"L2",    false, 0,    false},
        {0.13f,true, "D-L12", false, 0,    false},
        {0.43f,false,"L1",    false, 0,    false},
        /* thoracic */
        {0.11f,true, "D-T12", false, 0,    false},
        {0.40f,false,"T12",   false, 0,    true},
        {0.11f,true, "D-T11", false, 0,    false},
        {0.39f,false,"T11",   false, 0,    true},
        {0.11f,true, "D-T10", false, 0,    false},
        {0.38f,false,"T10",   false, 0,    true},
        {0.11f,true, "D-T9",  false, 0,    false},
        {0.37f,false,"T9",    false, 0,    true},
        {0.11f,true, "D-T8",  false, 0,    false},
        {0.36f,false,"T8",    false, 0,    true},
        {0.11f,true, "D-T7",  false, 0,    false},
        {0.35f,false,"T7",    false, 0,    true},
        {0.11f,true, "D-T6",  false, 0,    false},
        {0.34f,false,"T6",    false, 0,    true},
    };

    float y = 0.0f;
    for (auto &s : segs) {
        SpineLevel lv{};
        lv.y = y; lv.h = s.h; lv.is_disc = s.disc;
        lv.name = s.nm; lv.affected = s.aff;
        lv.shift_x = s.sx; lv.shift_z = 0;
        lv.thoracic = s.thor;
        if (s.disc) {
            /* disc diameter tapers from sacrum up */
            float frac = y / 8.0f;
            lv.w = 0.68f - frac*0.12f;
            lv.d = 0.58f - frac*0.10f;
        } else {
            float frac = y / 8.0f;
            float vw = strcmp(s.nm,"SACRUM")==0 ? 1.05f :
                       s.thor                    ? 0.62f - frac*0.08f :
                                                   0.80f - frac*0.05f;
            lv.w = vw;
            lv.d = strcmp(s.nm,"SACRUM")==0 ? 0.75f :
                   s.thor ? 0.50f : 0.65f;
        }
        levels.push_back(lv);
        y += s.h;
    }
}

/*  pulse factor (0-1 sine wave) */
static float pulse_factor() { return 0.5f+0.5f*sinf(g_time*3.5f); }

/* Draw a curved rib arc from (x0,y0) outward, X-ray style */
static void draw_rib_arc(float x0, float y0, float spread, float drop,
                          float thickness, bool left) {
    int segs = 22;
    float sx = left ? -1.0f : 1.0f;

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segs; i++) {
        float t  = (float)i / segs;
        float ang = t * 3.14159f;              /* 0..π */
        float rx  = x0 + sx * sinf(ang) * spread;
        float ry  = y0 - (1.0f-cosf(ang)) * drop * 0.5f
                       - sinf(ang) * drop * 0.5f;
        float rz  = -sinf(ang * 0.8f) * spread * 0.35f; /* depth bow */

        /* outward normal */
        float dtx = sx * cosf(ang) * spread;
        float dty = sinf(ang)*drop*0.5f + (1.0f-cosf(ang))*drop*0.5f;
        float len  = sqrtf(dtx*dtx + dty*dty) + 1e-6f;
        float nx = -dty/len * sx;
        float ny =  dtx/len;

        glVertex3f(rx + nx*thickness, ry + ny*thickness, rz);
        glVertex3f(rx - nx*thickness, ry - ny*thickness, rz);
    }
    glEnd();
}

/* Iliac wing (pelvic blade) – fan of triangles */
static void draw_iliac_wing(float cx, float cy, bool left) {
    float sx = left ? -1.0f : 1.0f;
    /* outer rim points of the iliac blade */
    static const float pts[][2] = {
        { 0.0f,  0.0f},   /* attach point near sacrum */
        { 0.5f,  0.4f},
        { 1.0f,  0.7f},
        { 1.5f,  0.8f},
        { 1.8f,  0.5f},
        { 1.9f,  0.0f},
        { 1.7f, -0.5f},
        { 1.2f, -0.8f},
        { 0.6f, -0.7f},
        { 0.15f,-0.4f},
    };
    int npts = (int)(sizeof(pts)/sizeof(pts[0]));

    /* filled wing */
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx + sx*0.55f, cy, 0.0f);  /* fan origin */
    for (int i=0; i<npts; i++) {
        glVertex3f(cx + sx*(0.55f+pts[i][0]), cy + pts[i][1], 0.0f);
    }
    glVertex3f(cx + sx*(0.55f+pts[0][0]), cy + pts[0][1], 0.0f);
    glEnd();

    /* darker inner marrow area */
    glColor4f(0.45f, 0.46f, 0.52f, 0.55f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx + sx*0.80f, cy+0.05f, 0.0f);
    for (int i=0; i<npts; i++) {
        float scale=0.55f;
        glVertex3f(cx + sx*(0.80f+pts[i][0]*scale),
                   cy + pts[i][1]*scale, 0.0f);
    }
    glVertex3f(cx + sx*(0.80f+pts[0][0]*0.55f), cy, 0.0f);
    glEnd();
}

/* Transverse + spinous process nubs on a vertebra */
static void draw_processes(float vw, float vd, float vh) {
    /* transverse processes (left & right) */
    glColor4f(0.78f, 0.79f, 0.84f, 0.88f);
    for (int side=-1; side<=1; side+=2) {
        glPushMatrix();
        glTranslatef(side*(vw*0.5f+0.18f), 0, 0);
        draw_box(0.36f, vh*0.28f, vh*0.28f);
        glPopMatrix();
    }
    /* spinous process (posterior spike) */
    glColor4f(0.72f, 0.73f, 0.80f, 0.85f);
    glPushMatrix();
    glTranslatef(0, 0, -(vd*0.5f+0.22f));
    draw_box(vh*0.22f, vh*0.22f, 0.44f);
    glPopMatrix();
}

/* ─── Full skeleton: spine + ribs + pelvis (X-ray style) ─────────────────── */
static void render_full_skeleton(const Patient &p) {
    std::vector<SpineLevel> levels;
    build_spine_levels(p, levels);

    float total_h = levels.back().y + levels.back().h;
    /* centre skeleton vertically at y=0 */
    glTranslatef(0, -total_h * 0.5f, 0);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    float pf = pulse_factor();

    /* ── 1. PELVIS / ILIAC WINGS ── */
    {
        float sacY = levels[0].y + levels[0].h * 0.5f;

        /* sacrum itself (drawn below) but wings now */
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.82f, 0.83f, 0.88f, 0.88f);
        draw_iliac_wing(0, sacY, true);
        draw_iliac_wing(0, sacY, false);

        /* pubic symphysis */
        glColor4f(0.80f, 0.81f, 0.86f, 0.80f);
        draw_box(1.2f, 0.22f, 0.22f);
    }

    /* ── 2. VERTEBRAL COLUMN ── */
    for (auto &lv : levels) {
        float sxoff = lv.shift_x;
        if (lv.affected && lv.shift_x != 0)
            sxoff += sinf(g_time*3.5f)*0.05f;

        glPushMatrix();
        glTranslatef(sxoff, lv.y + lv.h*0.5f, 0);

        if (lv.is_disc) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.16f, 0.17f, 0.20f, 0.85f);
            draw_cylinder(lv.w*0.44f, lv.h, 28);
        } else {
            float bright = (strcmp(lv.name,"SACRUM")==0) ? 0.95f
                         : lv.thoracic                   ? 0.82f
                                                         : 0.87f;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(bright, bright+0.01f, bright+0.03f, 0.95f);
            draw_box(lv.w, lv.h, lv.d);

            /* trabecular inner core */
            glColor4f(0.55f, 0.56f, 0.62f, 0.38f);
            draw_box(lv.w*0.70f, lv.h*0.75f, lv.d*0.70f);

            /* processes (not on sacrum) */
            if (strcmp(lv.name,"SACRUM") != 0) {
                glColor4f(bright*0.90f, bright*0.91f, bright*0.95f, 0.88f);
                draw_processes(lv.w, lv.d, lv.h);
            }
        }

        /* AFFECTED: red glow */
        if (lv.affected) {
            float a = 0.28f + pf * 0.40f;
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4f(1.0f, 0.08f + pf*0.06f, 0.05f, a);
            float rs = 1.0f + pf*0.08f;
            glScalef(rs, 1.0f, rs);
            if (lv.is_disc)
                draw_cylinder(lv.w*0.54f, lv.h*1.18f, 28);
            else
                draw_box(lv.w*1.10f, lv.h*1.06f, lv.d*1.10f);
            /* outer halo */
            glColor4f(1.0f, 0.22f, 0.05f, a*0.38f);
            glScalef(1.22f, 1.0f, 1.22f);
            if (lv.is_disc)
                draw_cylinder(lv.w*0.54f, lv.h, 28);
            else
                draw_box(lv.w*1.10f, lv.h*0.55f, lv.d*1.10f);
        }
        glPopMatrix();
    }

    /* ── 3. RIBS (attached to thoracic vertebrae) ── */
    for (auto &lv : levels) {
        if (!lv.thoracic) continue;
        float vy  = lv.y + lv.h * 0.5f;  /* vertebra centre Y */

        /* rib size scales with level – lower thoracic wider */
        float frac  = (vy - 5.0f) / 3.0f;  /* 0=upper T6, 1=lower T12 */
        frac = std::max(0.0f, std::min(1.0f, frac));
        float spread  = 1.6f + frac * 0.9f;   /* horizontal radius */
        float drop    = 0.5f + frac * 0.4f;   /* vertical drop */
        float thick   = 0.06f + frac * 0.02f;

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float rb = 0.78f + frac*0.08f;
        glColor4f(rb, rb+0.01f, rb+0.03f, 0.88f);

        draw_rib_arc(0, vy, spread, drop, thick, true);
        draw_rib_arc(0, vy, spread, drop, thick, false);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);
}

/* ═══════════════════════════════════════════════
   DETAIL: X-ray film 2D overlay
   (drawn ON TOP of the 3D spine in the left panel)
═══════════════════════════════════════════════ */
static void render_xray_film_overlay(const Patient &p) {
    int vw = g_win_w;   /* full window */
    int vh = g_win_h;

    push2d();
    glDisable(GL_LIGHTING);

    /* ── Scan-line film texture (subtle horizontal lines) ── */
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.055f);
    glBegin(GL_LINES);
    for (int sl = 0; sl < vh; sl += 3) {
        glVertex2f(0,(float)sl); glVertex2f((float)vw,(float)sl);
    }
    glEnd();
    glDisable(GL_BLEND);

    /* ── Film border ── */
    float bw = 10.0f;
    glColor3f(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(0,0);      glVertex2f(bw,0);      glVertex2f(bw,(float)vh); glVertex2f(0,(float)vh);
    glVertex2f(vw-bw,0);  glVertex2f((float)vw,0); glVertex2f((float)vw,(float)vh); glVertex2f(vw-bw,(float)vh);
    glVertex2f(0,vh-bw);  glVertex2f((float)vw,vh-bw); glVertex2f((float)vw,(float)vh); glVertex2f(0,(float)vh);
    glVertex2f(0,0);      glVertex2f((float)vw,0);  glVertex2f((float)vw,bw); glVertex2f(0,bw);
    glEnd();

    /* ── Corner cross-hair markers ── */
    glColor3f(0.35f, 0.38f, 0.44f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    /* top-left */  glVertex2f(bw+2,vh-bw-2); glVertex2f(bw+14,vh-bw-2); glVertex2f(bw+8,vh-bw+4); glVertex2f(bw+8,vh-bw-14);
    /* top-right */ glVertex2f(vw-bw-2,vh-bw-2); glVertex2f(vw-bw-14,vh-bw-2); glVertex2f(vw-bw-8,vh-bw+4); glVertex2f(vw-bw-8,vh-bw-14);
    /* bot-left */  glVertex2f(bw+2,bw+2); glVertex2f(bw+14,bw+2); glVertex2f(bw+8,bw-4); glVertex2f(bw+8,bw+14);
    /* bot-right */ glVertex2f(vw-bw-2,bw+2); glVertex2f(vw-bw-14,bw+2); glVertex2f(vw-bw-8,bw-4); glVertex2f(vw-bw-8,bw+14);
    glEnd();
    glLineWidth(1.0f);

    /* ── PATIENT ID – large, top-left ── */
    fill_rect_2d(bw+4,  vh-bw-62, 160, 50, 0.0f,0.0f,0.0f, 0.88f);
    fill_rect_2d(bw+4,  vh-bw-14, 160, 2,  0.55f,0.65f,0.9f, 1.0f); /* top accent */
    render_string_small(bw+10, vh-bw-30, "PATIENT ID", 0.40f, 0.45f, 0.55f);
    /* ID in large bold-ish text – draw twice for pseudo-bold */
    render_string_2d(bw+10, vh-bw-54, p.id, 0.95f, 0.97f, 0.55f);
    render_string_2d(bw+11, vh-bw-54, p.id, 0.95f, 0.97f, 0.55f); /* pseudo-bold */

    /* ── Age / Sex ── */
    char tmp[64];
    snprintf(tmp,sizeof(tmp),"AGE %d   SEX %s", p.age, p.sex);
    render_string_small(bw+10, vh-bw-76, tmp, 0.55f, 0.58f, 0.65f);

    /* ── Scan type – top-centre ── */
    render_string_small(vw/2-72, vh-bw-22, "LUMBAR  THORACIC  SPINE   AP VIEW", 0.42f, 0.45f, 0.52f);

    /* ── Date – bottom-left ── */
    snprintf(tmp,sizeof(tmp),"SCAN: %s", p.date);
    render_string_small(bw+8, bw+14, tmp, 0.32f, 0.35f, 0.40f);

    /* ── "R" laterality marker – top-right ── */
    fill_rect_2d(vw-bw-40, vh-bw-48, 30, 30, 0.0f,0.0f,0.0f, 0.82f);
    render_string_2d(vw-bw-35, vh-bw-44, "R", 0.85f, 0.87f, 0.92f);

    /* ── Vertebra level labels – y fractions matched to full skeleton ── */
    bool disk  = (strncmp(p.classification,"Disk",4)==0 ||
                  strncmp(p.classification,"Disc",4)==0 ||
                  strstr(p.classification,"Hern")!=nullptr);
    bool spond = strstr(p.classification,"Spondyl")!=nullptr;
    bool abn   = strcmp(p.classification,"Abnormal")==0;

    struct LvLbl { const char *name; float yf; bool aff; };
    LvLbl lbls[] = {
        {"S",  0.12f, false},
        {"L5", 0.22f, spond},
        {"L4", 0.28f, abn||disk},
        {"L3", 0.35f, abn},
        {"L2", 0.42f, false},
        {"L1", 0.49f, false},
        {"T12",0.56f, false},
        {"T10",0.64f, false},
        {"T8", 0.72f, false},
        {"T6", 0.80f, false},
    };
    /* skeleton centre is window centre; labels go to left of it */
    int lbl_x = vw/2 - 120;
    for (auto &lb : lbls) {
        int ly_px = (int)(lb.yf * vh);
        if (lb.aff) {
            /* red indicator dot */
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f,0.15f,0.08f,0.88f);
            glBegin(GL_QUADS);
            glVertex2f(lbl_x-14, ly_px-5); glVertex2f(lbl_x-2, ly_px-5);
            glVertex2f(lbl_x-2,  ly_px+7); glVertex2f(lbl_x-14,ly_px+7);
            glEnd();
            glDisable(GL_BLEND);
            render_string_small(lbl_x, ly_px-4, lb.name, 1.0f,0.25f,0.1f);
        } else {
            render_string_small(lbl_x, ly_px-4, lb.name, 0.50f,0.54f,0.60f);
        }
        /* horizontal tick line to spine */
        glColor3f(0.22f,0.24f,0.28f);
        glBegin(GL_LINES); glVertex2f(lbl_x+18,(float)ly_px); glVertex2f(lbl_x+30,(float)ly_px); glEnd();
    }

    /* ── Affected region annotation banner ── */
    if (disk || spond || abn) {
        float pf2 = pulse_factor();
        float fa  = 0.55f + pf2 * 0.45f;
        const char *note = disk  ? "  DISC HERNIATION DETECTED" :
                           spond ? "  SPONDYLOLISTHESIS DETECTED" :
                                   "  STRUCTURAL ABNORMALITY";
        fill_rect_2d(bw+4, bw+12, (float)(vw-2*bw-8), 28, 0.22f,0.03f,0.03f, 0.88f*fa);
        fill_rect_2d(bw+4, bw+38, (float)(vw-2*bw-8), 2,  1.0f,0.2f,0.1f,   fa);
        render_string_small(bw+12, bw+18, note, 1.0f, 0.30f+pf2*0.2f, 0.08f);
    }

    glEnable(GL_LIGHTING);
    pop2d();
}

/* ═══════════════════════════════════════════════
   DETAIL: info panel (right side)
═══════════════════════════════════════════════ */
static void render_detail_info(const Patient &p) {
    push2d();

    /* panel is 46% wide – fills the right side of the window */
    int pw = (int)(g_win_w * 0.46f);
    int x0 = g_win_w - pw;
    /* dark solid background */
    fill_rect_2d((float)x0, 0, (float)pw, (float)g_win_h, 0.03f,0.04f,0.10f, 0.97f);
    /* left-edge gradient fade */
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(0.03f,0.04f,0.10f,0.0f); glVertex2f((float)(x0-50),0); glVertex2f((float)(x0-50),(float)g_win_h);
    glColor4f(0.03f,0.04f,0.10f,0.97f); glVertex2f((float)x0,(float)g_win_h); glVertex2f((float)x0,0);
    glEnd();
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    /* risk accent bar at top */
    float pr = (strcmp(p.risk_cat,"HIGH")==0)?.9f:(strcmp(p.risk_cat,"MODERATE")==0)?.9f:.2f;
    float pg = (strcmp(p.risk_cat,"HIGH")==0)?.1f:(strcmp(p.risk_cat,"MODERATE")==0)?.7f:.9f;
    float pb = (strcmp(p.risk_cat,"HIGH")==0)?.0f:(strcmp(p.risk_cat,"MODERATE")==0)?.1f:.3f;
    fill_rect_2d((float)x0, (float)(g_win_h-8), (float)pw, 8.0f, pr,pg,pb,1.0f);

    int lx = x0+22;
    int ly = g_win_h - 36;
    /* TIMES_ROMAN_24 is ~26px tall → stride 38 */
    auto line = [&](const char *s, float r, float gg, float b) {
        render_string_2d((float)lx,(float)ly,s,r,gg,b); ly -= 38;
    };
    /* HELVETICA_18 is ~20px tall → stride 30 */
    auto small = [&](const char *s, float r, float gg, float b) {
        render_string_small((float)lx,(float)ly,s,r,gg,b); ly -= 30;
    };

    /* ── Prominent Patient ID block at top of panel ── */
    fill_rect_2d((float)x0, (float)(g_win_h-140), (float)pw, 132.0f, 0.06f,0.07f,0.18f, 1.0f);
    fill_rect_2d((float)x0, (float)(g_win_h-140), (float)pw, 3.0f,   pr,pg,pb, 1.0f);
    render_string_small((float)(x0+22), (float)(g_win_h-32), "PATIENT", 0.50f,0.54f,0.72f);
    /* ID – pseudo-bold with TIMES_ROMAN_24 */
    render_string_2d((float)(x0+22), (float)(g_win_h-72), p.id, 1.0f,1.0f,0.60f);
    render_string_2d((float)(x0+23), (float)(g_win_h-72), p.id, 1.0f,1.0f,0.60f);
    char agesex2[64];
    snprintf(agesex2,sizeof(agesex2),"Age %d   %s   %s", p.age, p.sex, p.date);
    render_string_small((float)(x0+22),(float)(g_win_h-110), agesex2, 0.62f,0.65f,0.75f);

    ly = g_win_h - 152;

    char buf[128];
    line("── DIAGNOSIS ───────────────────",0.7f,0.7f,1.0f);
    snprintf(buf,sizeof(buf),"  Class:  %s", p.classification);
    float cr  = (strcmp(p.risk_cat,"HIGH")==0)?1.0f:(strcmp(p.risk_cat,"MODERATE")==0)?.95f:.3f;
    float cg  = (strcmp(p.risk_cat,"HIGH")==0)?.15f:(strcmp(p.risk_cat,"MODERATE")==0)?.78f:.95f;
    float cb  = (strcmp(p.risk_cat,"HIGH")==0)?.1f:(strcmp(p.risk_cat,"MODERATE")==0)?.1f:.35f;
    line(buf,cr,cg,cb);
    snprintf(buf,sizeof(buf),"  Risk  :  %.1f%%   [ %s ]", p.risk_score, p.risk_cat);
    line(buf,cr,cg,cb);

    /* risk bar */
    int bx=lx, by=ly; ly-=22;
    fill_rect_2d((float)bx,(float)by,(float)(pw-44),18, 0.1f,0.12f,0.18f);
    float filled = (pw-44) * p.risk_score / 100.0f;
    fill_rect_2d((float)bx,(float)by,filled,18, cr,cg,cb,0.9f);
    ly -= 10;

    snprintf(buf,sizeof(buf),"  Abn prob: %.0f%%", p.abnorm_pct);
    line(buf,cr*0.9f,cg*0.9f,cb*0.9f);

    ly -= 4;
    line("── CLINICAL SCORES ─────────────",0.7f,0.7f,1.0f);
    snprintf(buf,sizeof(buf),"  Cobb angle     : %.1f deg", p.cobb_angle);
    float cobbclr = p.cobb_angle>25?.9f:p.cobb_angle>10?.9f:.3f;
    float cobbclrg= p.cobb_angle>25?.1f:p.cobb_angle>10?.75f:.9f;
    small(buf,cobbclr,cobbclrg,.1f);
    snprintf(buf,sizeof(buf),"  Bone T-score   : %.1f", p.bone_density);
    float bclr=(p.bone_density<-2.5f)?1.0f:(p.bone_density<-1.0f)?.9f:.3f;
    float bclrg=(p.bone_density<-2.5f)?.15f:(p.bone_density<-1.0f)?.7f:.9f;
    small(buf,bclr,bclrg,.1f);
    snprintf(buf,sizeof(buf),"  Fracture grade : %d / 3", p.frac_grade);
    small(buf,p.frac_grade>=2?1.0f:.7f,p.frac_grade>=2?.15f:.7f,.5f);
    snprintf(buf,sizeof(buf),"  Disc degen     : %d / 4", p.disc_grade);
    small(buf,p.disc_grade>=3?1.0f:.7f,p.disc_grade>=3?.15f:.7f,.5f);

    ly -= 12;
    line("── X-RAY FILM KEY ──────────────────",0.7f,0.7f,1.0f);
    fill_rect_2d((float)lx,(float)(ly-3),18,18,0.88f,0.89f,0.92f); render_string_small((float)(lx+26),(float)ly,"Vertebral body",.6f,.6f,.65f); ly-=30;
    fill_rect_2d((float)lx,(float)(ly-3),18,18,0.18f,0.19f,0.22f); render_string_small((float)(lx+26),(float)ly,"Disc space (dark gap)",.6f,.6f,.65f); ly-=30;
    fill_rect_2d((float)lx,(float)(ly-3),18,18,0.9f,0.12f,0.05f);  render_string_small((float)(lx+26),(float)ly,"Pathological region",.9f,.3f,.15f); ly-=30;

    ly -= 12;
    line("── PROBLEM DESCRIPTION ─────────",0.7f,0.7f,1.0f);
    if (strncmp(p.classification,"Disk",4)==0||strncmp(p.classification,"Disc",4)==0||strstr(p.classification,"Hern")) {
        small("  Intervertebral disc herniation",.9f,.5f,.2f);
        small("  detected at L4-L5 & L5/S1",.8f,.5f,.2f);
        small("  disc presses on nerve root",.7f,.4f,.2f);
    } else if (strstr(p.classification,"Spondyl")) {
        small("  Spondylolisthesis: L5 vertebra",.9f,.7f,.2f);
        small("  slipped forward on sacrum",.8f,.6f,.2f);
        small("  causing instability / nerve pain",.7f,.5f,.2f);
    } else if (strcmp(p.classification,"Abnormal")==0) {
        small("  Multi-level spinal abnormality",.9f,.2f,.2f);
        small("  L3/L4 structural compromise",.8f,.2f,.2f);
        small("  High fracture / compression risk",.7f,.2f,.2f);
    } else {
        small("  No significant abnormality",.3f,.9f,.3f);
        small("  detected in this scan",.3f,.8f,.3f);
    }

    /* back button – bigger to match large fonts */
    float btnx2=(float)x0+22, btny2=46;
    fill_rect_2d(btnx2,btny2-4,170,38,0.15f,0.2f,0.35f,0.95f);
    fill_rect_2d(btnx2,btny2+32,170,3, pr,pg,pb,0.8f);
    render_string_2d(btnx2+18,btny2+8,"[ \xe2\x86\x90 Back ]",0.8f,0.9f,1.0f);

    pop2d();
}

/* ═══════════════════════════════════════════════
   TOOLTIP (overview hover)
═══════════════════════════════════════════════ */
static void render_tooltip() {
    if (g_hover < 0 || g_hover >= (int)g_patients.size()) return;
    const Patient &p = g_patients[g_hover];

    /* Panel dimensions – wider/taller for TIMES_ROMAN_24 / HELVETICA_18 */
    const float PW = 360.0f;
    const float PH = 240.0f;
    const float PAD = 14.0f;

    /* Anchor to the right of the cursor; flip if near right edge */
    float tx = (float)g_mouse_x + 18;
    float ty_top = (float)g_mouse_y - 8;
    if (tx + PW + 4 > g_win_w) tx = (float)g_mouse_x - PW - 14;
    if (ty_top + PH > g_win_h) ty_top = (float)g_win_h - PH - 4;
    if (ty_top < 4) ty_top = 4;

    /* Convert to OpenGL coords (Y flipped) */
    float by = (float)g_win_h - ty_top - PH;

    /* Risk colour */
    float cr, cg;
    bool isHigh = strcmp(p.risk_cat,"HIGH")==0;
    bool isMod  = strcmp(p.risk_cat,"MODERATE")==0;
    if (isHigh)      { cr=1.00f; cg=0.20f; }
    else if (isMod)  { cr=1.00f; cg=0.80f; }
    else             { cr=0.20f; cg=0.95f; }

    push2d();

    /* ── outer shadow ── */
    fill_rect_2d(tx+5, by-5, PW, PH, 0,0,0, 0.55f);

    /* ── main panel background ── */
    fill_rect_2d(tx, by, PW, PH, 0.05f,0.07f,0.14f, 0.96f);

    /* ── top accent bar ── */
    fill_rect_2d(tx, by+PH-5, PW, 5, cr, cg, 0.1f, 1.0f);

    /* ── left risk stripe ── */
    fill_rect_2d(tx, by, 5, PH, cr, cg, 0.1f, 0.85f);

    /* ── panel border ── */
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
    glColor4f(cr*0.5f, cg*0.4f, 0.3f, 0.7f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(tx,by); glVertex2f(tx+PW,by); glVertex2f(tx+PW,by+PH); glVertex2f(tx,by+PH);
    glEnd();
    glEnable(GL_LIGHTING);

    char buf[92];
    float lx = tx + PAD + 6;

    /* "PATIENT" label  (HELVETICA_18, ~20px tall) */
    render_string_small(lx, by+PH-28,  "PATIENT",        0.45f,0.50f,0.65f);
    /* Patient ID  (TIMES_ROMAN_24, ~26px tall) – pseudo-bold */
    render_string_2d   (lx, by+PH-68,  p.id,             1.0f, 1.0f, 0.55f);
    render_string_2d   (lx+1, by+PH-68,p.id,             1.0f, 1.0f, 0.55f);

    /* divider */
    fill_rect_2d(lx, by+PH-80, PW-PAD*2-6, 1, 0.25f,0.30f,0.45f, 0.9f);

    /* Age / sex  (HELVETICA_18, 28px line stride) */
    snprintf(buf,sizeof(buf),"Age: %d   Sex: %s", p.age, p.sex);
    render_string_small(lx, by+PH-108, buf, 0.75f,0.80f,0.90f);

    /* Classification */
    snprintf(buf,sizeof(buf),"Dx: %s", p.classification);
    render_string_small(lx, by+PH-138, buf, cr, cg, 0.15f);

    /* Risk score */
    snprintf(buf,sizeof(buf),"Risk Score:  %.0f%%", p.risk_score);
    render_string_small(lx, by+PH-168, buf, cr, cg, 0.4f);

    /* Abnormality */
    snprintf(buf,sizeof(buf),"Abnormality: %.0f%%", p.abnorm_pct);
    render_string_small(lx, by+PH-198, buf, 0.75f, 0.75f, 0.85f);

    /* Risk category badge (top-right corner) */
    float bx2 = tx + PW - 96;
    fill_rect_2d(bx2, by+PH-70, 82, 24, cr,cg,0.08f, 0.28f);
    render_string_small(bx2+8, by+PH-63, p.risk_cat, cr, cg, 0.1f);

    /* ── footer: click hint ── */
    fill_rect_2d(tx+4, by+4, PW-8, 28, 0.08f,0.10f,0.20f, 0.85f);
    render_string_small(lx, by+10, "Click pillar  ->  open X-RAY scan", 0.40f,0.55f,0.80f);

    pop2d();
}

/* ═══════════════════════════════════════════════
   OVERVIEW title bar
═══════════════════════════════════════════════ */
static void render_overview_hud() {
    push2d();
    fill_rect_2d(0,(float)(g_win_h-46),(float)g_win_w,46, 0.04f,0.05f,0.12f,0.88f);
    fill_rect_2d(0,(float)(g_win_h-48),(float)g_win_w,2,  0.3f,0.4f,0.8f,1.0f);

    char buf[128];
    render_string_2d(14,(float)(g_win_h-30),
        "KARNIX SPINE OS  \xe2\x80\x94  Patient Risk Heatmap (3D)",
        0.85f,0.9f,1.0f);
    snprintf(buf,sizeof(buf),"Patients: %d    DB: PostgreSQL    Drag=rotate   Scroll=zoom   Click=select",
        (int)g_patients.size());
    render_string_small(14,(float)(g_win_h-46),buf, .5f,.6f,.75f);

    /* legend */
    float lx=(float)(g_win_w-360), ly=(float)(g_win_h-22);
    fill_rect_2d(lx,ly-4,14,14, 1,.22f,.18f); render_string_small(lx+18,ly,"HIGH   ",.9f,.4f,.3f);
    fill_rect_2d(lx+80,ly-4,14,14, 1,.78f,.1f); render_string_small(lx+98,ly,"MODERATE",.9f,.8f,.3f);
    fill_rect_2d(lx+190,ly-4,14,14, .2f,.95f,.35f); render_string_small(lx+208,ly,"LOW",.3f,.9f,.4f);
    pop2d();
}

/* ═══════════════════════════════════════════════
   Ray-picking for overview pillars
═══════════════════════════════════════════════ */
static int pick_pillar(double mx, double my) {
    /* Get matrices */
    double mvmat[16], prjmat[16];
    int viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,  mvmat);
    glGetDoublev(GL_PROJECTION_MATRIX, prjmat);
    glGetIntegerv(GL_VIEWPORT, viewport);

    double ox,oy,oz, dx,dy,dz;
    gluUnProject(mx, g_win_h-my, 0, mvmat,prjmat,viewport, &ox,&oy,&oz);
    gluUnProject(mx, g_win_h-my, 1, mvmat,prjmat,viewport, &dx,&dy,&dz);
    dx-=ox; dy-=oy; dz-=oz;

    /* Intersect ray with y=0 plane */
    if (fabs(dy)<1e-6) return -1;
    double t = -oy/dy;
    double hx = ox + dx*t;
    double hz = oz + dz*t;

    /* Check each pillar AABB */
    int best=-1; double best_dist=1e9;
    for (int i=0; i<(int)g_patients.size(); i++) {
        const Patient &p = g_patients[i];
        float wx=pillar_world_x(p), wz=pillar_world_z(p);
        if (fabs(hx-wx)<0.55 && fabs(hz-wz)<0.55) {
            double d=sqrt((hx-wx)*(hx-wx)+(hz-wz)*(hz-wz));
            if (d<best_dist){best_dist=d;best=i;}
        }
    }
    return best;
}

/* ═══════════════════════════════════════════════
   GLFW callbacks
═══════════════════════════════════════════════ */
static void cb_mouse_button(GLFWwindow*, int btn, int action, int) {
    if (btn==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS) {
        double mx,my; glfwGetCursorPos(g_win,&mx,&my);

        if (g_detail_view && g_selected>=0) {
            /* "Back" button: x0+12 to x0+132, y g_win_h-(btny+26) to g_win_h-btny */
            int pw = (int)(g_win_w*0.36f), x0=g_win_w-pw;
            float btnx=x0+12, btny=34;
            if (mx>=btnx && mx<=btnx+120 && my>=(g_win_h-btny-26) && my<=(g_win_h-btny+2)) {
                g_detail_view=false; g_selected=-1; return;
            }
        }

        if (!g_detail_view) {
            int idx = pick_pillar(mx,my);
            if (idx>=0) {
                g_selected   = idx;
                g_detail_view= true;
                g_detail_spin= 0;
            } else {
                g_drag=true; g_drag_last_x=mx; g_drag_last_y=my;
            }
        }
    }
    if (btn==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_RELEASE)
        g_drag=false;
}

static void cb_cursor(GLFWwindow*, double mx, double my) {
    g_mouse_x=mx; g_mouse_y=my;
    if (g_drag && !g_detail_view) {
        float dx=(float)(mx-g_drag_last_x);
        float dy=(float)(my-g_drag_last_y);
        g_cam_yaw   += dx*0.35f;
        g_cam_pitch += dy*0.25f;
        g_cam_pitch = std::max(-5.0f, std::min(85.0f, g_cam_pitch));
        g_drag_last_x=mx; g_drag_last_y=my;
    }
    /* hover: find nearest pillar screen-coord captured last frame */
    if (!g_detail_view && !g_pillar_screen.empty()) {
        g_hover = -1;
        float best2 = 60.0f*60.0f;   /* 60 px radius */
        for (int i=0; i<(int)g_pillar_screen.size(); i++) {
            float ddx = g_pillar_screen[i].sx - (float)mx;
            float ddy = g_pillar_screen[i].sy - (float)my;
            float d2  = ddx*ddx + ddy*ddy;
            if (d2 < best2) { best2=d2; g_hover=i; }
        }
    }
}

static void cb_scroll(GLFWwindow*, double, double dy) {
    if (!g_detail_view) {
        g_cam_dist -= (float)dy*1.2f;
        g_cam_dist  = std::max(6.0f, std::min(60.0f, g_cam_dist));
    }
}

static void cb_key(GLFWwindow*, int key, int, int action, int) {
    if (action!=GLFW_PRESS) return;
    if (key==GLFW_KEY_ESCAPE) {
        if (g_detail_view) { g_detail_view=false; g_selected=-1; }
        else glfwSetWindowShouldClose(g_win, GLFW_TRUE);
    }
    /* Arrow keys to navigate selected patient */
    if (!g_detail_view) {
        int n=(int)g_patients.size();
        if (key==GLFW_KEY_LEFT  && g_selected>0)   g_selected--;
        if (key==GLFW_KEY_RIGHT && g_selected<n-1) g_selected++;
        if (key==GLFW_KEY_UP    && g_selected>=8)  g_selected-=8;
        if (key==GLFW_KEY_DOWN  && g_selected+8<n) g_selected+=8;
        if (key==GLFW_KEY_ENTER && g_selected>=0)  { g_detail_view=true; g_detail_spin=0; }
    }
}

static void cb_resize(GLFWwindow*, int w, int h) {
    g_win_w=w; g_win_h=h;
    glViewport(0,0,w,h);
}

/* ═══════════════════════════════════════════════
   Scene setup helpers
═══════════════════════════════════════════════ */
static void setup_lighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    float amb[4]={0.08f,0.08f,0.14f,1};
    float diff0[4]={0.9f,0.9f,1.0f,1};
    float spec0[4]={0.7f,0.7f,0.9f,1};
    float pos0[4]={8,18,6,0};    /* directional */

    float diff1[4]={0.25f,0.3f,0.6f,1};
    float pos1[4]={-10,4,-8,0};  /* fill */

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff1);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
}

static void set_camera_overview() {
    float yaw   = g_cam_yaw   * 3.14159f/180.0f;
    float pitch = g_cam_pitch * 3.14159f/180.0f;
    float cx = g_cam_dist * cosf(pitch) * sinf(yaw);
    float cy = g_cam_dist * sinf(pitch);
    float cz = g_cam_dist * cosf(pitch) * cosf(yaw);
    gluLookAt(cx,cy,cz, 0,1,0, 0,1,0);
}

static void set_camera_detail() {
    gluLookAt(0, 0, 14.0f,  /* closer so skeleton fills screen */
              0, 0, 0,
              0, 1, 0);
}

/* sky background gradient */
static void draw_sky() {
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(0.04f,0.04f,0.10f); glVertex2f(-1,-1); glVertex2f(1,-1);
    glColor3f(0.07f,0.09f,0.22f); glVertex2f(1,1);   glVertex2f(-1,1);
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* particle stars */
static void draw_stars() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    unsigned int seed=42;
    for (int i=0;i<180;i++) {
        seed=seed*6364136223846793005ULL+1;
        float sx=((seed>>17)&0xFFFF)/65535.0f*2-1;
        seed=seed*6364136223846793005ULL+1;
        float sy=((seed>>17)&0xFFFF)/65535.0f*2-1;
        float bright=0.3f+0.7f*(float)(i%5)/4.0f;
        float twinkle=0.7f+0.3f*sinf(g_time*1.8f+(float)i);
        glColor3f(bright*twinkle,bright*twinkle,bright);
        glVertex3f(sx*60,sy*30+10,-55);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

/* ═══════════════════════════════════════════════
   Main render
═══════════════════════════════════════════════ */
static void render_frame() {
    float dt = g_time - g_prev_time;
    g_prev_time = g_time;

    /* ── Animate bar rise ── */
    for (auto &p : g_patients) {
        float t = (g_time - p.rise_delay);
        if (t < 0) t=0;
        p.bar_cur = p.bar_target * spring(t * 1.4f);
    }

    /* ── Detail spine: gentle rock ±14° to show depth without spinning ── */
    if (g_detail_view)
        g_detail_spin = 14.0f * sinf(g_time * 0.35f);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    draw_sky();
    draw_stars();

    float aspect=(float)g_win_w/(float)g_win_h;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, aspect, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();

    setup_lighting();

    if (!g_detail_view) {
        /* ── OVERVIEW ── */
        set_camera_overview();
        /* slow auto-rotate camera when idle */
        g_cam_yaw += dt * 4.0f;
        render_overview();   /* also fills g_pillar_screen this frame */
        render_overview_hud();
        if (g_hover >= 0) render_tooltip();
    } else {
        /* ── DETAIL: X-ray film – full window ── */
        if (g_selected>=0 && g_selected<(int)g_patients.size()) {
            const Patient &p = g_patients[g_selected];

            /* 1. Fill whole window with X-ray film black */
            push2d();
            glDisable(GL_LIGHTING);
            glColor3f(0.012f, 0.012f, 0.018f);
            glBegin(GL_QUADS);
            glVertex2f(0,0); glVertex2f((float)g_win_w,0);
            glVertex2f((float)g_win_w,(float)g_win_h); glVertex2f(0,(float)g_win_h);
            glEnd();
            glEnable(GL_LIGHTING);
            pop2d();

            /* 2. Render 3D skeleton in FULL window */
            glViewport(0, 0, g_win_w, g_win_h);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            gluPerspective(68.0, (double)g_win_w/g_win_h, 0.1, 80.0);
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            set_camera_detail();
            setup_lighting();
            glClear(GL_DEPTH_BUFFER_BIT);

            glPushMatrix();
            glScalef(2.2f, 2.2f, 2.2f);  /* fill the window */
            glRotatef(g_detail_spin, 0,1,0);
            render_full_skeleton(p);
            glPopMatrix();

            /* 3. Full-window X-ray film overlay (borders, labels, ID) */
            render_xray_film_overlay(p);

            /* 4. Info panel floats as semi-transparent overlay on right */
            render_detail_info(p);
        }
    }
}

/* ═══════════════════════════════════════════════
   main
═══════════════════════════════════════════════ */
int main(int argc, char **argv) {
    /* GLUT init (only for bitmap fonts – no GLUT window) */
    glutInit(&argc, argv);

    /* Load data */
    fprintf(stdout,"[spine3d] Loading patient data...\n");
    load_from_db();
    setup_patient_visuals();
    fprintf(stdout,"[spine3d] %d patients ready\n",(int)g_patients.size());

    /* GLFW init */
    if (!glfwInit()) { fprintf(stderr,"glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_SAMPLES,4);  /* 4× MSAA */
    glfwWindowHint(GLFW_DOUBLEBUFFER,GLFW_TRUE);

    g_win = glfwCreateWindow(g_win_w, g_win_h,
        "KarnixOS – Spine 3D Visualizer  (Click a pillar to inspect)",
        nullptr, nullptr);
    if (!g_win) { glfwTerminate(); fprintf(stderr,"Window create failed\n"); return 1; }
    glfwMakeContextCurrent(g_win);
    glfwSwapInterval(1);  /* vsync */

    glfwSetMouseButtonCallback(g_win, cb_mouse_button);
    glfwSetCursorPosCallback  (g_win, cb_cursor);
    glfwSetScrollCallback     (g_win, cb_scroll);
    glfwSetKeyCallback        (g_win, cb_key);
    glfwSetFramebufferSizeCallback(g_win, cb_resize);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.04f,0.04f,0.10f,1);

    fprintf(stdout,
        "\n  Controls:\n"
        "    Click a pillar  -> open 3D spine detail\n"
        "    Arrow keys      -> navigate patients (when one is selected)\n"
        "    Enter           -> open selected patient detail\n"
        "    Drag mouse      -> rotate overview camera\n"
        "    Scroll          -> zoom\n"
        "    Esc             -> back / quit\n\n");

    while (!glfwWindowShouldClose(g_win)) {
        g_time = (float)glfwGetTime();
        render_frame();
        glfwSwapBuffers(g_win);
        glfwPollEvents();
    }

    glfwDestroyWindow(g_win);
    glfwTerminate();
    return 0;
}
