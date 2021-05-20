#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <iostream>

// Screen constants
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 1000;
const int FRAME_RATE_LIMIT = 30;
const int BASE_SPAWN_MARGIN = 20;
const int MOUSE_CLICK_SPAWN_RANGE = 20;

// Particles
const int PARTICLES_COUNT = 200;
const int MOUSE_CLICK_PARTICLES_SPAWN_COUNT = 20;
bool FREEZE_PARTICLES_ON_COLLAPSE = false;
bool FREEZE_PARTICLES_ON_BORDER_COLLAPSE = false;
int LAST_PARTICLE_ID = 0;
const int MIN_RADIUS = 1;
const int MAX_RADIUS = 5;
// Gravity
bool GRAVITY_ENABLED = false;
const sf::Vector2f GRAVITY_FORCE(0.f, 1.f);
// Time 
float TIME = 0.5;
int SECONDS = 0;
int FRAMES = 0;
// Controls
bool PAUSED = false;
// Colors
const int COLORS_LENGTH = 7;
sf::Color COLORS[COLORS_LENGTH] = { sf::Color::White, sf::Color::Green, sf::Color::Blue, sf::Color::Yellow, sf::Color::Red, sf::Color::Magenta, sf::Color::Cyan };

// Types
typedef struct {
    int id;
    float radius;
    bool freeze;
    bool removed;
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::CircleShape shape;
} particle;

typedef struct {
    bool collapsed;
    particle* collapsedParticle;
} particleCollapsed_t;

// Functions 
void initParticles(int n, std::vector<particle>& particles);
void renderParticles(sf::RenderWindow& window, std::vector<particle>& particles);
void updateParticles(std::vector<particle>& particles);
int borderCollapse(particle p);
float randomFloat(float min, float max);
particleCollapsed_t particleCollapse(particle& p, std::vector<particle>& particles);
void clearParticles(std::vector<particle>& particles);
void reloadParticles(std::vector<particle>& particles);
void spawnMoreParticlesOnMousePositionRange(sf::Vector2i mousePosition, std::vector<particle>& particles);
bool inParticleCollapsePositionRange(sf::Vector2f positionA, sf::Vector2f positionB);
void removeOffScreenParticles(std::vector<particle>& particles);
void clearRemovedParticlesAndReallocate(std::vector<particle>& particles);
particle createParticle(int id, float radius, bool freeze, sf::Vector2f position, sf::Vector2f velocity, sf::Color color);
sf::Color randomColor();
void resolveCollision(particle& p, particle& p2);


// Main workflow
int main()
{
    srand(static_cast<unsigned>(time(0)));
    sf::VideoMode videoMode = sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT);
    sf::RenderWindow window(videoMode, "Particles");
    window.setPosition(sf::Vector2i(1520, 0));
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(FRAME_RATE_LIMIT);

    std::vector<particle> particles;

    // initParticles(PARTICLES_COUNT, particles);

    particles.push_back(createParticle(1, 10, false, sf::Vector2f(500.f, 500.f), sf::Vector2f(-1.f, 0.f), sf::Color::Red));
    particles.push_back(createParticle(1, 10, false, sf::Vector2f(400.f, 500.f), sf::Vector2f(1.f, 0.f), sf::Color::Yellow));

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::MouseButtonPressed) {
                switch (event.mouseButton.button)
                {
                case sf::Mouse::Left:
                    spawnMoreParticlesOnMousePositionRange(sf::Mouse::getPosition(window), particles);
                    break;
                default:
                    break;
                }
            }
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code)
                {
                case sf::Keyboard::F:
                    FREEZE_PARTICLES_ON_COLLAPSE = !FREEZE_PARTICLES_ON_COLLAPSE;
                    break;
                case sf::Keyboard::B:
                    FREEZE_PARTICLES_ON_BORDER_COLLAPSE = !FREEZE_PARTICLES_ON_BORDER_COLLAPSE;
                    break;
                case sf::Keyboard::G:
                    GRAVITY_ENABLED = !GRAVITY_ENABLED;
                    break;
                case sf::Keyboard::R:
                    reloadParticles(particles);
                    break;
                case sf::Keyboard::Space:
                    PAUSED = !PAUSED;
                    break;
                case sf::Keyboard::C:
                    clearParticles(particles);
                    break;
                case sf::Keyboard::Right:
                    TIME = TIME == 0.1 ? 0.5 : 1.f;
                    break;
                case sf::Keyboard::Left:
                    TIME = TIME == 1.f ? 0.5 : 0.1;
                    break;
                default:
                    break;
                }
            }
        }
        FRAMES++;
        if (FRAMES >= FRAME_RATE_LIMIT) {
            SECONDS++;
            FRAMES = 0;
            clearRemovedParticlesAndReallocate(particles);
        }
        if (PAUSED) {
            continue;
        }
        updateParticles(particles);
        removeOffScreenParticles(particles);
        window.clear();
        renderParticles(window, particles);
        window.display();
    }

    return 0;
}

// Function bodies

sf::Color randomColor() {
    return COLORS[(rand() % COLORS_LENGTH) - 1];
}

void clearRemovedParticlesAndReallocate(std::vector<particle>& particles) {
    for (auto it = particles.begin(); it != particles.end();) {
        if (it->removed) {
            it = particles.erase(it);
        }
        else {
            ++it;
        }
    }
}

void removeOffScreenParticles(std::vector<particle>& particles) {
    for (auto& p : particles) {
        bool offX = p.position.x + p.radius * 2 < 0.f || p.position.x - p.radius * 2 > WINDOW_WIDTH;
        bool offY = p.position.y + p.radius * 2 < 0.f || p.position.y - p.radius * 2 > WINDOW_HEIGHT;
        p.removed = offX || offY;
    }
}

void spawnMoreParticlesOnMousePositionRange(sf::Vector2i mousePosition, std::vector<particle>& particles) {
    for (int i = 0; i < MOUSE_CLICK_PARTICLES_SPAWN_COUNT; i++) {
        float minX = mousePosition.x - MOUSE_CLICK_SPAWN_RANGE;
        if (minX <= 0) {
            minX = mousePosition.x + BASE_SPAWN_MARGIN;
        }
        float maxX = mousePosition.x + MOUSE_CLICK_SPAWN_RANGE;
        if (maxX >= WINDOW_WIDTH) {
            maxX = mousePosition.x - BASE_SPAWN_MARGIN;
        }
        float minY = mousePosition.y - MOUSE_CLICK_SPAWN_RANGE;
        if (minY <= 0) {
            minX = mousePosition.y + BASE_SPAWN_MARGIN;
        }
        float maxY = mousePosition.y + MOUSE_CLICK_SPAWN_RANGE;
        if (maxY >= WINDOW_HEIGHT) {
            maxX = mousePosition.y - BASE_SPAWN_MARGIN;
        }
        particle p = createParticle(
            i,
            randomFloat(MIN_RADIUS, MAX_RADIUS),
            false,
            sf::Vector2f(randomFloat(minX, maxX), randomFloat(minY, maxY)),
            sf::Vector2f(randomFloat(-10, 10), randomFloat(-10, 10)), randomColor()
        );
        particles.push_back(p);
    }
}

void reloadParticles(std::vector<particle>& particles) {
    clearParticles(particles);
    initParticles(PARTICLES_COUNT, particles);
}

void clearParticles(std::vector<particle>& particles) {
    particles.clear();
}

void initParticles(int n, std::vector<particle>& particles) {
    for (int i = 0; i < n; i++) {
        particle p = createParticle(
            i,
            randomFloat(MIN_RADIUS, MAX_RADIUS),
            false,
            sf::Vector2f(randomFloat(BASE_SPAWN_MARGIN, WINDOW_WIDTH - BASE_SPAWN_MARGIN), randomFloat(BASE_SPAWN_MARGIN, WINDOW_HEIGHT - BASE_SPAWN_MARGIN)),
            sf::Vector2f(randomFloat(-10, 10), randomFloat(-10, 10)),
            randomColor()
        );
        particles.push_back(p);
    }
}

void renderParticles(sf::RenderWindow& window, std::vector<particle>& particles) {
    for (auto& p : particles) {
        window.draw(p.shape);
    }
}

void resolveCollision(particle& p, particle& p2) {
    float vx1 = (p.velocity.x * 0.1f + 2 * p2.velocity.x) / 2, f;
    float vx2 = (p2.velocity.x * 0.1f + 2 * p.velocity.x) / 2.f;
    p.velocity = sf::Vector2f(vx1, p.velocity.y);
    p2.velocity = sf::Vector2f(vx1, p2.velocity.y);

}

void updateParticles(std::vector<particle>& particles) {
    for (auto& p : particles) {
        if (p.removed) {
            continue;
        }
        int borderCollapsed = borderCollapse(p);
        if (borderCollapsed != 0) {
            p.freeze = FREEZE_PARTICLES_ON_BORDER_COLLAPSE;
        }
        particleCollapsed_t particleCollapsed = particleCollapse(p, particles);
        if (particleCollapsed.collapsed) {
            p.freeze = FREEZE_PARTICLES_ON_COLLAPSE;
            particleCollapsed.collapsedParticle->freeze = FREEZE_PARTICLES_ON_COLLAPSE;
            resolveCollision(p, *particleCollapsed.collapsedParticle);
        }
        if (!p.freeze) {
            if (GRAVITY_ENABLED) {
                p.velocity += GRAVITY_FORCE * TIME;
            }
            p.position += p.velocity * TIME;
        }
        p.shape.setPosition(p.position);
    }
}

int borderCollapse(particle p) {
    if (p.removed) {
        return 0;
    }
    const float diameter = p.radius * 2;
    if (p.position.x + diameter > WINDOW_WIDTH) {
        return 2;
    }
    if (p.position.x - p.radius < 0) {
        return -2;
    }
    if (p.position.y - diameter < 0) {
        return -1;
    }
    if (p.position.y + diameter > WINDOW_HEIGHT) {
        return 1;
    }
    return 0;
}

float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

float distanceBetweenTwoPoints(sf::Vector2f a, sf::Vector2f b) {
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

bool inParticleCollapsePositionRange(sf::Vector2f positionA, sf::Vector2f positionB) {
    float rangeX = positionA.x + MAX_RADIUS * 2;
    float rangeY = positionA.y + MAX_RADIUS * 2;
    return positionB.x <= rangeX && positionB.x >= -rangeX && positionB.y <= rangeY && positionB.y >= -rangeY;
}

particleCollapsed_t particleCollapse(particle& p, std::vector<particle>& particles) {
    particleCollapsed_t result;
    result.collapsed = false;
    for (auto& otherParticle : particles) {
        if (otherParticle.removed || !inParticleCollapsePositionRange(p.position, otherParticle.position)) {
            continue;
        }
        if (otherParticle.id == p.id) {
            return result;
        }
        float distance = distanceBetweenTwoPoints(p.position, otherParticle.position);
        if (distance < p.radius + otherParticle.radius) {
            result.collapsed = true;
            result.collapsedParticle = &otherParticle;
            return result;
        }
    }
    return result;
}

particle createParticle(int id, float radius, bool freeze, sf::Vector2f position, sf::Vector2f velocity, sf::Color color) {
    particle p;
    p.removed = false;
    p.id = LAST_PARTICLE_ID + id;
    if (p.id >= LAST_PARTICLE_ID) {
        LAST_PARTICLE_ID = p.id;
    }
    p.radius = radius;
    p.freeze = freeze;
    p.position = position;
    p.velocity = velocity;
    p.shape = sf::CircleShape(p.radius);
    p.shape.setFillColor(color);
    p.shape.setPosition(p.position);
    return p;
}