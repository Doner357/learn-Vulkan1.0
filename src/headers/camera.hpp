#ifndef LEARN_VK_ARCBALL_CAMERA
#define LEARN_VK_ARCBALL_CAMERA

#define GLM_ENABLE_EXPERIMENTAL
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
            updateCamera();
        }

        // Mouse scroll for zoom in/out
        void radiusMove(float yoffset) {
            glm::vec3 new_pos = position + cam_front * yoffset * mouse_sensitive;
            if (glm::length(new_pos - look_at) > 0.0001f) {
                position = new_pos;
                updateCamera();
            }
        }

        // Mouse movement for rotation
        void spherePosMove(float xoffset, float yoffset) {
            xoffset *= mouse_sensitive;
            yoffset *= mouse_sensitive;

            glm::quat yaw = glm::angleAxis(glm::radians(xoffset), cam_up);
            glm::quat pitch = glm::angleAxis(glm::radians(yoffset), cam_right);
            glm::quat combined_rotation = pitch * yaw;

            // Rotate position around look_at
            position = combined_rotation * (position - look_at) + look_at;
            updateCamera();
        }

        // Look point movement
        void centerMove(float xoffset, float yoffset) {
            xoffset *= speed;
            yoffset *= speed;

            glm::vec3 offset = yoffset * cam_up + xoffset * cam_right;
            look_at += offset;
            position += offset;
            updateCamera();
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

        // Camera matrices
        glm::mat4 view_mat;
        glm::mat4 projection;
        glm::mat4 view_to_clip;

        // Update camera vectors and matrices
        void updateCamera() {
            cam_front = glm::normalize(look_at - position);
            cam_right = glm::normalize(glm::cross(cam_front, cam_up));
            cam_up = glm::normalize(glm::cross(cam_right, cam_front));
            view_mat = glm::lookAt(position, look_at, cam_up);
            projection = glm::perspective(glm::radians(fov), aspect, near, far);
            view_to_clip = projection * view_mat;
        }
    };

} // namespace lvk

#endif // LEARN_VK_ARCBALL_CAMERA
