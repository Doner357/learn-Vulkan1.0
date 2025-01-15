#ifndef LEARN_VK_ARCBALL_CAMERA
#define LEARN_VK_ARCBALL_CAMERA

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lvk {

    class ArcballCamera {
    public:
        ArcballCamera() = default;

        ArcballCamera(
            glm::vec3 position,
            glm::vec3 look_at,
            glm::vec3 cam_up,
            float fov,
            float aspect,
            float near,
            float far,
            float speed = 1.0f,
            float mouse_sensitive = 0.1f
        ) : position(position),
            look_at(look_at),
            cam_up(glm::normalize(cam_up)),
            fov(fov),
            aspect(aspect),
            near(near),
            far(far),
            speed(speed),
            mouse_sensitive(mouse_sensitive)
        {
            up_side_down = cam_up.y < 0;
            updateCamera();
        }

        // Mouse scroll for zoom in/out
        void radiusMove(float yoffset);

        // Mouse movement for rotation
        void spherePosMove(float xoffset, float yoffset);

        // Look point movement
        void centerMove(float xoffset, float yoffset);

        void checkUpSide() {
            up_side_down = cam_up.y < 0;
        }

        // Setters
        void setPosition(const glm::vec3& pos) { position = pos; updateCamera(); }
        void setLookAt(const glm::vec3& la) { look_at = la; updateCamera(); }
        void setUp(const glm::vec3& up) { cam_up = glm::normalize(up); updateCamera(); }
        void setFov(float fov) { if (fov < 180.0f) this->fov = fov; updateCamera(); }
        void setAspect(float aspect) { if (aspect > 0.0f) this->aspect = aspect; updateCamera(); }
        void setNearPlane(float near) { this->near = near; updateCamera(); }
        void setFarPlane(float far) { this->far = far; updateCamera(); }
        void setSpeed(float spd) { speed = glm::max(spd, 0.0f); }
        void setMouseSensitive(float sensitive) { mouse_sensitive = sensitive; }

        // Getters
        glm::vec3 getPosition() const { return position; }
        glm::vec3 getLookAt() const { return look_at; }
        glm::vec3 getCamUp() const { return cam_up; }
        glm::vec3 getCamFront() const { return cam_front; }
        glm::vec3 getCamRight() const { return cam_right; }
        float getFov() const { return fov; }
        float getAspect() const { return aspect; }
        float getNearPlane() const { return near; }
        float getFarPlane() const { return far; }
        float getSpeed() const { return speed; }
        float getMouseSensitive() const { return mouse_sensitive; }
        glm::mat4 getViewMat() const { return view_mat; }
        glm::mat4 getProjectionMat() const { return projection; }
        glm::mat4 getViewToClipMat() const { return view_to_clip; }

    private:
        // Camera attributes
        glm::vec3 position;
        glm::vec3 look_at;
        glm::vec3 cam_up;
        glm::vec3 cam_front;
        glm::vec3 cam_right;

        // Projection attributes
        float fov;
        float aspect;
        float near;
        float far;

        // Control attributes
        float speed;
        float mouse_sensitive;

        // Upside down flag
        bool up_side_down;

        // Camera matrices
        glm::mat4 view_mat;
        glm::mat4 projection;
        glm::mat4 view_to_clip;

        // Update camera vectors and matrices
        void updateCamera(bool use_self_right = false);
    };

} // namespace lvk

#endif // LEARN_VK_ARCBALL_CAMERA
