#include "camera.hpp"

namespace lvk {

    void ArcballCamera::radiusMove(float yoffset) {
        glm::vec3 new_pos = position + cam_front * yoffset * mouse_sensitive;
        if (glm::length(new_pos - look_at) > 0.0001f) {
            position = new_pos;
            updateCamera();
        }
    }

    // Mouse movement for rotation
    void ArcballCamera::spherePosMove(float xoffset, float yoffset) {
        xoffset *= mouse_sensitive * ((up_side_down) ? -1.0f : 1.0f);
        yoffset *= mouse_sensitive;

        glm::mat3 yaw = glm::rotate(glm::mat4(1.0f), glm::radians(xoffset), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat3 pitch = glm::rotate(glm::mat4(1.0f), glm::radians(yoffset), cam_right);
        glm::mat3 combined_rotation = pitch * yaw;
        cam_up = pitch * cam_up;
        cam_right = yaw * cam_right;

        // Rotate position around look_at
        position = combined_rotation * (position - look_at) + look_at;
        updateCamera(true);
    }

    // Look point movement
    void ArcballCamera::centerMove(float xoffset, float yoffset) {
        xoffset *= speed;
        yoffset *= speed;

        glm::vec3 view_up = glm::normalize(glm::cross(cam_right, cam_front));
        glm::vec3 offset = yoffset * view_up + xoffset * cam_right;
        look_at += offset;
        position += offset;
        updateCamera();
    }

    void ArcballCamera::updateCamera(bool use_self_right) {
        cam_front = glm::normalize(look_at - position);
        glm::vec3 temp_right = use_self_right ? cam_right : glm::cross(cam_front, cam_up);
        cam_right = glm::normalize(glm::vec3(temp_right.x, 0.0f, temp_right.z));
        cam_up = glm::normalize(glm::cross(cam_right, cam_front));
        if (use_self_right) {
            cam_right = glm::normalize(glm::cross(cam_front, cam_up));
        }
        view_mat = glm::lookAt(position, look_at, cam_up);
        projection = glm::perspective(glm::radians(fov), aspect, near, far);
        view_to_clip = projection * view_mat;
    }

}
