#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <box2d/box2d.h>
#include <box2d/id.h>
#include <box2d/math_functions.h>
#include <box2d/types.h>
#include <cstddef>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_internal.h>

// font for ImGui
ImFont *ImFont_3270;

// type of the enemy
enum class entity_type { floor, character, bullet };

// declaration of entity struct, needed for entity typizator
struct entity;

// entity typizator
struct entity_tpz {
    entity_type type;
    size_t vecpos;
    entity_tpz(entity_type type, size_t vecpos) : type(type), vecpos(vecpos) {}
};

// a struct of an entity - a floor, a character or a bullet object
struct entity : public sf::RectangleShape {
    b2BodyId b2bid; // box2d id
    entity_tpz *tpz =
        new entity_tpz(entity_type::character,
                       0); // entity typizator, needed for collision processing (down below)
    entity(sf::Vector2f size, b2Vec2 init_pos, b2Rot init_rot, entity_type etype, b2WorldId b2wid,
           size_t vecpos, b2Vec2 init_speed = {0, 0})
        : sf::RectangleShape(size) {
        tpz->type = etype;    // don't forget to actually set type of the object
        tpz->vecpos = vecpos; // and set it's vector position
        float restitution = 0.2; // needed down below
        b2BodyDef bodyDef = b2DefaultBodyDef(); // needed for initializing a body
        switch (tpz->type) {
        case entity_type::floor:
            bodyDef.type = b2_staticBody;
            setFillColor(sf::Color(127, 0, 127));
            break;
        case entity_type::bullet:
            setFillColor(sf::Color(255, 0, 0));
            bodyDef.isBullet = true; // this stuff is neat
            bodyDef.type = b2_dynamicBody;
            restitution = 0.8;
            break;
        case entity_type::character:
            setFillColor(sf::Color(0, 127, 127));
            bodyDef.type = b2_dynamicBody;
            bodyDef.angularDamping = 3.9;
            bodyDef.linearDamping = 0.5;
            break;
        default:
            break;
        }
        bodyDef.userData = tpz; // setting a typizator as a box2d body's user data
        bodyDef.position = init_pos;
        bodyDef.rotation = init_rot;
        bodyDef.linearVelocity = init_speed;
        b2bid = b2CreateBody(b2wid, &bodyDef); 
        b2Polygon Box = b2MakeBox(size.x / 2, size.y / 2);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.1f;
        shapeDef.material.restitution = restitution;
        if (tpz->type != entity_type::floor)
            shapeDef.enableContactEvents = true;
        b2CreatePolygonShape(b2bid, &shapeDef, &Box);
        setOrigin(getLocalBounds().getCenter());
    }
    void sync() {
        b2Vec2 position = b2Body_GetPosition(b2bid);
        b2Rot rotation = b2Body_GetRotation(b2bid);
        float pr_rotation = -b2Rot_GetAngle(rotation);
        setPosition({position.x, -position.y});
        setRotation(sf::radians(pr_rotation));
    }
    b2BodyId return_b2bid() { return b2bid; }
};

struct world_t {
  public:
    b2WorldId b2wid; // box2d world id
    std::vector<entity> entities;
} world;

void init() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, -9.8f};
    world.b2wid = b2CreateWorld(&worldDef);
    world.entities.push_back({{1, 1},
                              {-1, 0},
                              b2MakeRot(3.14 / 4.004),
                              entity_type::character,
                              world.b2wid,
                              world.entities.size()});
    world.entities.push_back({{1, 1},
                              {1, 0},
                              b2MakeRot(3.14 / 4.004),
                              entity_type::character,
                              world.b2wid,
                              world.entities.size()});
    world.entities.push_back({{14, 0.25},
                              {0, -3.375},
                              b2MakeRot(0),
                              entity_type::floor,
                              world.b2wid,
                              world.entities.size()});
    world.entities.push_back({{14, 0.25},
                              {0, 3.375},
                              b2MakeRot(0),
                              entity_type::floor,
                              world.b2wid,
                              world.entities.size()});
    world.entities.push_back({{0.25, 7},
                              {5.875, 0},
                              b2MakeRot(0),
                              entity_type::floor,
                              world.b2wid,
                              world.entities.size()});
    world.entities.push_back({{0.25, 7},
                              {-5.875, 0},
                              b2MakeRot(0),
                              entity_type::floor,
                              world.b2wid,
                              world.entities.size()});
}

void brush_entities() {
    for (size_t x = 0; x < world.entities.size(); x++) {
        world.entities[x].tpz->vecpos = x;
    }
}

void proc_contact_events() {

    b2ContactEvents contact_events = b2World_GetContactEvents(world.b2wid);
    b2ContactBeginTouchEvent *cebe = contact_events.beginEvents;
    for (int i = 0; i < contact_events.beginCount; i++) {
        entity_tpz *etpz_x;
        entity_tpz *etpz_y;
        b2BodyId body_id_x = b2Shape_GetBody(cebe[i].shapeIdA);
        b2BodyId body_id_y = b2Shape_GetBody(cebe[i].shapeIdB);
        void *data_x = b2Body_GetUserData(body_id_x);
        void *data_y = b2Body_GetUserData(body_id_y);
        auto tpz_x = reinterpret_cast<entity_tpz *>(data_x);
        auto tpz_y = reinterpret_cast<entity_tpz *>(data_y);
        if (tpz_x->type == entity_type::bullet) {
            b2DestroyBody(world.entities[tpz_x->vecpos].b2bid);
            world.entities.erase(world.entities.begin() + tpz_x->vecpos);
            brush_entities();
        }
        if (tpz_y->type == entity_type::bullet) {
            b2DestroyBody(world.entities[tpz_y->vecpos].b2bid);
            world.entities.erase(world.entities.begin() + tpz_y->vecpos);
            brush_entities();
        }

        brush_entities();
    }
}

void update(float dt, sf::RenderWindow *wndw) {
    b2World_Step(world.b2wid, dt, 4);
    proc_contact_events();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left, true)) {
        b2Body_ApplyLinearImpulseToCenter(world.entities[0].b2bid, {-1.5, 0}, true);
        b2Vec2 bot_pos = b2Body_GetPosition(world.entities[0].b2bid);
        sf::Vector2i mouse_pos_pix = sf::Mouse::getPosition(*wndw);
        sf::Vector2i mouse_pos =
            wndw->mapCoordsToPixel({(float)mouse_pos_pix.x, (float)mouse_pos_pix.y});
        b2Vec2 init_bullet_pos = {bot_pos.x + 1, bot_pos.y};
        world.entities.push_back({{0.1, 0.1},
                                  init_bullet_pos,
                                  b2MakeRot(0),
                                  entity_type::bullet,
                                  world.b2wid,
                                  world.entities.size(),
                                  {100, 0}});
    }

    for (size_t i = 0; i < world.entities.size(); i++)
        world.entities[i].sync();
}

void draw(sf::RenderWindow *window) {
    for (size_t i = 0; i < world.entities.size(); i++)
        window->draw(world.entities[i]);
}

void imgui_debug_window() {
    ImGui::PushFont(ImFont_3270);
    ImGui::Begin("debug");
    ImGui::Text("Sample Text");
    ImGui::Text("Тестовый Текст");
    ImGui::Text("1234567890");
    ImGui::Text("!\"@$;^:&?*()");
    ImGui::End();
    ImGui::PopFont();
}

int main() {
    sf::ContextSettings contxt_sttngs;
    contxt_sttngs.antiAliasingLevel = 8;
    contxt_sttngs.depthBits = 24;
    contxt_sttngs.majorVersion = 4;
    contxt_sttngs.minorVersion = 6;
    contxt_sttngs.sRgbCapable = false;
    sf::RenderWindow wndw(sf::VideoMode({1280, 720}), "gaem", sf::Style::Close, sf::State::Windowed,
                          contxt_sttngs);
    wndw.setFramerateLimit(144);
    if (!ImGui::SFML::Init(wndw))
        return -1;
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImFont_3270 = io.Fonts->AddFontFromFileTTF("../3270NerdFont.ttf", 36, nullptr,
                                               io.Fonts->GetGlyphRangesCyrillic());
    if (!ImGui::SFML::UpdateFontTexture())
        return -1;
    sf::Clock clock;
    sf::Time looptime = sf::seconds(0.1);

    init();
    sf::View viw = wndw.getDefaultView();
    viw.setSize({12, 7});
    viw.setCenter({0, 0});
    wndw.setView(viw);

    while (wndw.isOpen()) {
        while (const std::optional event = wndw.pollEvent()) {
            ImGui::SFML::ProcessEvent(wndw, *event);
            if (event->is<sf::Event::Closed>() || ImGui::IsKeyDown(ImGuiKey_Backspace)) {
                wndw.close();
            }
        }

        looptime = clock.restart();
        ImGui::SFML::Update(wndw, looptime);
        update(looptime.asSeconds(), &wndw);
        imgui_debug_window();
        wndw.clear({64, 64, 64});
        draw(&wndw);
        ImGui::SFML::Render(wndw);
        wndw.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}