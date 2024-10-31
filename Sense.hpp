#pragma once

struct Sense {
    ConfigLoader* cl;
    LocalPlayer* lp;
    std::vector<Player*>* players;
    Camera* gameCamera;
    AimBot* aimBot;
    bool shouldUnload = false;
    bool settingsSaved = false;

    Sense(ConfigLoader* in_cl, LocalPlayer* in_lp, std::vector<Player*>* in_players, Camera* in_gameCamera, AimBot* in_aimBot) {
        this->cl = in_cl;
        this->lp = in_lp;
        this->players = in_players;
        this->gameCamera = in_gameCamera;
        this->aimBot = in_aimBot;
    }

    void renderStatus(bool leftLock, bool rightLock, bool autoFire, int boneId, double averageProcessingTime, double averageFPS, int cache) {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 25.0f), ImGuiCond_Once, ImVec2(0.02f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.50f);
        ImGui::Begin("Status", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoInputs);

        const ImVec4 leftLockColor = leftLock ? ImVec4(0.4, 1, 0.343, 1) : ImVec4(1, 0.343, 0.475, 1);
        const ImVec4 rightLockColor = rightLock ? ImVec4(0.4, 1, 0.343, 1) : ImVec4(1, 0.343, 0.475, 1);
        const ImVec4 autoFireColor = autoFire ? ImVec4(0.4, 1, 0.343, 1) : ImVec4(1, 0.343, 0.475, 1);
        const ImVec4 boneIdColor = ImVec4(1, 1, 0.343, 1);
        const ImVec4 processingTimeColor = averageProcessingTime < 20 ? ImVec4(0.4, 1, 0.343, 1) : ImVec4(1, 0.343, 0.475, 1);
        ImGui::TextColored(leftLockColor, "< ");
        ImGui::SameLine();
        ImGui::TextColored(autoFireColor, "^ ");
        ImGui::SameLine();
        ImGui::TextColored(rightLockColor, "> ");
        ImGui::SameLine();
        ImGui::Text("hitbox: ");
        ImGui::SameLine();
        if (boneId == 0) ImGui::TextColored(boneIdColor, "HEAD ");
        else if (boneId == 1) ImGui::TextColored(boneIdColor, "NECK ");
        else ImGui::TextColored(boneIdColor, "BODY ");
        ImGui::SameLine();
        ImGui::Text("interval: ");
        ImGui::SameLine();
        ImGui::TextColored(processingTimeColor, "%.2fms ", averageProcessingTime);
        if (cl->FEATURE_SUPER_GLIDE_ON) {
            ImGui::SameLine();
            ImGui::Text("fps: %.2f ", averageFPS);
        }
        ImGui::SameLine();
        ImGui::Text("esp: %s ", data::items[data::selectedRadio][0].c_str());
        ImGui::SameLine();
        ImGui::Text("cache: %d", cache);
        ImGui::End();
    }

    void drawText(ImDrawList* canvas, Vector2D textPosition, const char* text, ImVec4 textColor, float scaleFactor) {
        int textX = textPosition.x;
        int textY = textPosition.y;
        char buffer[16];
        strncpy(buffer, text, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0'; // Ensure null-termination
        const auto textSize = ImGui::CalcTextSize(buffer);
        const auto horizontalOffset = textSize.x / 2;
        const auto verticalOffset = textSize.y - 20;

        // Draw background
        glColor4f(0, 0, 0, 0.00f);
        glBegin(GL_QUADS);
        glVertex2f(textX - horizontalOffset - 3, textY + 3);
        glVertex2f(textX + horizontalOffset + 1, textY + 3);
        glVertex2f(textX + horizontalOffset + 1, textY + textSize.y - 1);
        glVertex2f(textX - horizontalOffset - 3, textY + textSize.y - 1);
        glEnd();

        // Draw text
        const auto textColorImColor = ImColor(textColor);
        canvas->AddText({ textPosition.x - horizontalOffset, textPosition.y - verticalOffset }, textColorImColor, buffer);
    }

    bool isPlayerOnScreen(Player* player) {
        Vector2D localOriginW2S, headPositionW2S;
        Vector3D localOrigin3D = player->localOrigin;
        Vector3D headPosition3D = player->getBonePosition(HitboxType::Head);

        bool isLocalOriginW2SValid = gameCamera->worldToScreen(localOrigin3D, localOriginW2S);
        bool isHeadPositionW2SValid = gameCamera->worldToScreen(headPosition3D, headPositionW2S);

        return isLocalOriginW2SValid && isHeadPositionW2SValid;
    }

    void renderESP(ImDrawList* canvas) {
        Vector2D drawPosition;
        Vector2D screenSize = gameCamera->getResolution();
        bool drawVisibleWarning = false;
        bool drawDroneWarning = false;
        for (int i = 0; i < players->size(); i++) {
            Player* p = players->at(i);
            if (!p->isItem && !cl->SENSE_SHOW_DEAD && !p->currentHealth > 0) continue;

            Vector2D localOriginW2S, headPositionW2S, aboveHeadW2S;
            Vector3D localOrigin3D = p->localOrigin;
            Vector3D headPosition3D;
            if (!p->isPlayer && !p->isDrone && !p->isDummie) {
                headPosition3D = localOrigin3D;
                headPosition3D.z += 10.0f;
            } else { headPosition3D = p->getBonePosition(HitboxType::Head); }
            Vector3D aboveHead3D = headPosition3D;
            aboveHead3D.z += 10.0f;

            bool isLocalOriginW2SValid = gameCamera->worldToScreen(localOrigin3D, localOriginW2S);
            bool isHeadPositionW2SValid = gameCamera->worldToScreen(headPosition3D, headPositionW2S);
            gameCamera->worldToScreen(aboveHead3D, aboveHeadW2S);

            if (!isLocalOriginW2SValid || !isHeadPositionW2SValid) continue;

            // Colors - Players (Enemy)
            ImVec4 enemyBoxColor;
            if (p->isDrone || p->isItem) enemyBoxColor = ImVec4(1.00f, 0.00f, 1.00f, 1.00f);
            else if (p->isKnocked) enemyBoxColor =       ImVec4(1.00f, 0.67f, 0.17f, 1.00f);
            else if (p->isVisible) enemyBoxColor =       ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
            else enemyBoxColor =                         ImVec4(1.00f, 0.00f, 0.00f, 1.00f);

            float distance = util::inchesToMeters(p->distance2DToLocalPlayer);
            if (p->isEnemy && p->isValid() && !p->isLocal && distance < cl->SENSE_MAX_RANGE) {

                // Draw Boxes
                if (cl->SENSE_SHOW_BOX && isLocalOriginW2SValid && isHeadPositionW2SValid) {
                    Vector2D foot = localOriginW2S;
                    Vector2D head = headPositionW2S;
                    float height = head.y - foot.y;
                    float width = height / 2;

                    // Calculate the corner size based on the distance
                    float cornerSize = 7.0f; // Base corner size
                    if (distance > 100) {
                        cornerSize = 5.0f; // Adjust the size of the corners as needed
                    }
                    if (distance > 150) {
                        cornerSize = 3.0f; // Further reduce the corner size
                    }
                    if (distance > 200) {
                        cornerSize = 1.0f; // Minimum corner size to form a "+"
                    }

                    glColor4f(enemyBoxColor.x, enemyBoxColor.y, enemyBoxColor.z, 1.0f);
                    glLineWidth(1.5f);
                    glBegin(GL_LINES);

                    // Bottom-left corner
                    glVertex2f(foot.x - width / 2, foot.y);
                    glVertex2f(foot.x - width / 2, foot.y - cornerSize);
                    glVertex2f(foot.x - width / 2, foot.y);
                    glVertex2f(foot.x - width / 2 - cornerSize, foot.y);

                    // Bottom-right corner
                    glVertex2f(foot.x + width / 2, foot.y);
                    glVertex2f(foot.x + width / 2, foot.y - cornerSize);
                    glVertex2f(foot.x + width / 2, foot.y);
                    glVertex2f(foot.x + width / 2 + cornerSize, foot.y);

                    // Top-left corner
                    glVertex2f(head.x - width / 2, head.y + height / 5);
                    glVertex2f(head.x - width / 2 - cornerSize, head.y + height / 5);
                    glVertex2f(head.x - width / 2, head.y + height / 5);
                    glVertex2f(head.x - width / 2, head.y + height / 5 + cornerSize);

                    // Top-right corner
                    glVertex2f(head.x + width / 2, head.y + height / 5);
                    glVertex2f(head.x + width / 2 + cornerSize, head.y + height / 5);
                    glVertex2f(head.x + width / 2, head.y + height / 5);
                    glVertex2f(head.x + width / 2, head.y + height / 5 + cornerSize);

                    glEnd();
                }

                //360 Alert
                // if (cl->SENSE_360_ALERT && (!isLocalOriginW2SValid || !isHeadPositionW2SValid)) {
                //     Vector2D screenCenter(screenSize.x / 2, screenSize.y / 2);
                //     Vector2D direction = localOriginW2S.Subtract(screenCenter);  // Direction from center to off-screen player
                    
                //     float angle = atan2(direction.y, direction.x);  // Calculate angle in radians

                //     if (angle < -M_PI) angle += 2 * M_PI;
                //     if (angle > M_PI) angle -= 2 * M_PI;

                //     Vector2D edgePos;
                //     std::string txtDistance = std::to_string((int)distance);
                //     std::string indicator;
                    
                //     if (abs(angle) < M_PI / 4) {  // Right edge
                //         edgePos.x = screenSize.x;
                //         edgePos.y = screenCenter.y + tan(angle) * screenCenter.x;
                //         indicator = txtDistance + " >";  // Right side
                //     } else if (abs(angle) > 3 * M_PI / 4) {  // Left edge
                //         edgePos.x = 0;
                //         edgePos.y = screenCenter.y - tan(angle) * screenCenter.x;
                //         indicator = "< " + txtDistance;  // Left side
                //     } else if (angle > M_PI / 4 && angle <= 3 * M_PI / 4) {
                //         edgePos.x = screenCenter.x + tan(M_PI / 2 - angle) * screenCenter.y;
                //         edgePos.y = screenSize.y;
                //         indicator = txtDistance + "\nv";  // Bottom side / behind
                //     }

                //     if (!indicator.empty()) {
                //         ImVec4 indicatorColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                //         drawText(canvas, edgePos, indicator.c_str(), indicatorColor, 1.0f);
                //     }
                // } NOT WORKING YET(FUK)


                // Draw bar
                if (cl->SENSE_SHOW_PLAYER_BARS && !p->isItem && isLocalOriginW2SValid && isHeadPositionW2SValid) {
                    Vector2D foot = localOriginW2S;
                    Vector2D head = headPositionW2S;
                    float height = head.y - foot.y;
                    float width = height / 2;
                    int life = p->currentHealth;
                    int evo = p->currentShield;
                    if (evo > 100)     glColor3f(1.00f, 0.25f, 0.00f); // Red shield
                    else if (evo > 75) glColor3f(1.00f, 0.25f, 1.00f); // Purple shield
                    else if (evo > 50) glColor3f(0.00f, 0.75f, 1.00f); // Blue shield
                    else if (evo > 0)  glColor3f(1.00f, 1.00f, 1.00f); // White shield
                    else               glColor3f(1.00f, 1.00f, 0.00f); // No shield

                    glLineWidth(5.0f);
                    glBegin(GL_LINES);
                    glVertex2f(head.x + width/2 - 5.0f, foot.y);
                    glVertex2f(head.x + width/2 - 5.0f, foot.y + (height + height/5) * life/100);
                    glEnd();
                }

                if (cl->SENSE_TEXT_BOTTOM)
                    drawPosition = localOriginW2S.Subtract(Vector2D(0, 10));
                else
                    drawPosition = aboveHeadW2S.Subtract(Vector2D(0, 10));

                // Draw Distance
                if (cl->SENSE_SHOW_PLAYER_DISTANCES && isLocalOriginW2SValid && isHeadPositionW2SValid) {
                    if (cl->SENSE_TEXT_BOTTOM)
                        drawPosition = drawPosition.Add(Vector2D(0, 20));
                    else
                        drawPosition = drawPosition.Subtract(Vector2D(0, 20));

                    const char* txtPrefix = "[";
                    const char* txtDistance = std::to_string((int)distance).c_str();
                    const char* txtSuffix = " M]";
                    char distanceText[256];
                    strncpy(distanceText, txtPrefix, sizeof(distanceText));
                    strncat(distanceText, txtDistance, sizeof(distanceText));
                    strncat(distanceText, txtSuffix, sizeof(distanceText));

                    // Calculate the scaling factor based on the distance and SENSE_MAX_RANGE
                    float scaleFactor = 1.0f - (distance / cl->SENSE_MAX_RANGE);
                    scaleFactor = std::max(0.5f, std::min(1.0f, scaleFactor)); // Clamp scaleFactor between 0.5 and 1.0

                    // Adjust the font size using the scaling factor
                    ImVec4 scaledColor = ImVec4(enemyBoxColor.x, enemyBoxColor.y, enemyBoxColor.z, enemyBoxColor.w * scaleFactor);
                    drawText(canvas, drawPosition, distanceText, scaledColor, scaleFactor);
                }

                // Draw Name
                if (cl->SENSE_SHOW_PLAYER_NAMES || p->isItem) {
                    if (cl->SENSE_TEXT_BOTTOM)
                        drawPosition = drawPosition.Add(Vector2D(0, 20));
                    else
                        drawPosition = drawPosition.Subtract(Vector2D(0, 20));
                    const char* txtName;
                    if (p->isPlayer)
                        txtName = p->getPlayerName().c_str();
                    else if (p->isDrone)
                        txtName = "Drone";
                    else if (p->isDummie)
                        txtName = "Dummie";
                    else
                        for (int arraySize = sizeof(data::items) / sizeof(data::items[0]), i = 0; i < arraySize; i++)
                            if (p->itemId == stoi(data::items[i][1])) { txtName = data::items[i][0].c_str(); break; }
                    char nameText[256];
                    strncpy(nameText, txtName, sizeof(nameText));

                    // Calculate the scaling factor based on the distance and SENSE_MAX_RANGE
                    float scaleFactor = 1.0f - (distance / cl->SENSE_MAX_RANGE);
                    scaleFactor = std::max(0.5f, std::min(1.0f, scaleFactor)); // Clamp scaleFactor between 0.5 and 1.0

                    // Adjust the font size using the scaling factor
                    ImVec4 scaledColor = ImVec4(enemyBoxColor.x, enemyBoxColor.y, enemyBoxColor.z, enemyBoxColor.w * scaleFactor);
                    drawText(canvas, drawPosition, nameText, scaledColor, scaleFactor);
                }

                // Draw Level
                if (cl->SENSE_SHOW_PLAYER_LEVELS && p->isPlayer) {
                    if (cl->SENSE_TEXT_BOTTOM)
                        drawPosition = drawPosition.Add(Vector2D(0, 20));
                    else
                        drawPosition = drawPosition.Subtract(Vector2D(0, 20));
                    const char* txtPrefix = "Lv ";
                    const char* txtLevel = std::to_string(p->GetPlayerLevel()).c_str();
                    char levelText[256];
                    strncpy(levelText, txtPrefix, sizeof(levelText));
                    strncat(levelText, txtLevel, sizeof(levelText));

                    // Calculate the scaling factor based on the distance and SENSE_MAX_RANGE
                    float scaleFactor = 1.0f - (distance / cl->SENSE_MAX_RANGE);
                    scaleFactor = std::max(0.5f, std::min(1.0f, scaleFactor)); // Clamp scaleFactor between 0.5 and 1.0

                    // Adjust the font size using the scaling factor
                    ImVec4 scaledColor = ImVec4(enemyBoxColor.x, enemyBoxColor.y, enemyBoxColor.z, enemyBoxColor.w * scaleFactor);
                    drawText(canvas, drawPosition, levelText, scaledColor, scaleFactor);
                }

                // Draw Warning
                if (p->isDrone)
                    drawDroneWarning = true;
                else
                    if (p->isVisible && !p->isKnocked)
                        drawVisibleWarning = true;
            }
        }

        if (drawVisibleWarning) {
            ImVec4 warningColor = ImColor(ImVec4(0.00f, 1.00f, 0.00f, 1.00f));
            drawPosition = Vector2D(screenSize.x / 2, screenSize.y * 3/4);
            const char* txtWarning;
            txtWarning = "VISIBLE WARNING";
            char warningText[256];
            strncpy(warningText, txtWarning, sizeof(warningText));
            drawText(canvas, drawPosition, warningText, warningColor, 1.0f);
        }

        if (drawDroneWarning) {
            ImVec4 warningColor = ImColor(ImVec4(1.00f, 0.00f, 1.00f, 1.00f));
            drawPosition = Vector2D(screenSize.x / 2, screenSize.y * 3/4 + 20);
            const char* txtWarning;
            txtWarning = "DRONE WARNING";
            char warningText[256];
            strncpy(warningText, txtWarning, sizeof(warningText));
            drawText(canvas, drawPosition, warningText, warningColor, 1.0f);
        }

        // Draw FOV circle
        if (cl->SENSE_SHOW_FOV) {
            ImVec2 center(screenSize.x / 2, screenSize.y / 2);
            int baseRadius = screenSize.y / 60 * cl->AIMBOT_FOV;
            int radius = baseRadius + baseRadius * (60 / lp->zoomFov - 1) * cl->AIMBOT_FOV_EXTRA_BY_ZOOM;
            if (cl->SENSE_SHOW_FOV_AIM) {
                if (lp->inZoom) {
                    canvas->AddCircle(center, radius, ImColor(ImVec4(1.00f, 1.00f, 1.00f, 1.00f)));
                }
            } else {
                canvas->AddCircle(center, radius, ImColor(ImVec4(1.00f, 1.00f, 1.00f, 1.00f)));
            }
        }

        // Draw target line
        if (cl->SENSE_SHOW_TARGET && aimBot->targetSelected) {
            Vector2D targetBoneW2S;
            gameCamera->worldToScreen(aimBot->targetBone3DCache, targetBoneW2S);
            glColor3f(1.00f, 1.00f, 1.00f);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(screenSize.x / 2, screenSize.y / 2);
            glVertex2f(targetBoneW2S.x, targetBoneW2S.y);
            glEnd();
        }
    }

    static Vector3D rotatePoint(Vector3D localPlayerPos, Vector3D playerPos, int posX, int posY, 
        int sizeX, int sizeY, float playerAngle, float zoom, float scale, bool* viewCheck) {
        float r_1, r_2;
        float x_1, y_1;

        r_1 = -(playerPos.y - localPlayerPos.y);
        r_2 = playerPos.x - localPlayerPos.x;

        float yawToRadian = (-playerAngle - 90) * (M_PI / 180.0f);

        x_1 = (float)(r_2 * cos(yawToRadian) - r_1 * sin(yawToRadian)) / 20;
        y_1 = (float)(r_2 * sin(yawToRadian) + r_1 * cos(yawToRadian)) / 20;

        *viewCheck = y_1 < 0;

        // Apply zoom and scale
        x_1 *= zoom * scale;
        y_1 *= zoom * scale;

        x_1 += sizeX / 2;
        y_1 += sizeY / 2;

        if (x_1 < 5)
            x_1 = 5;
        if (x_1 > sizeX - 5)
            x_1 = sizeX - 5;
        if (y_1 < 5)
            y_1 = 5;
        if (y_1 > sizeY - 5)
            y_1 = sizeY - 5;

        x_1 += posX;
        y_1 += posY;

        return Vector3D(x_1, y_1, 0);
    }

    Vector3D WorldToRadar(const Vector3D& enemyPos, const Vector3D& localPos, float localYaw, const ImVec2& radarCenter, float radarScale, const ImVec2& radarSize) {
        Vector3D delta;
        delta.x = enemyPos.x - localPos.x;
        delta.y = enemyPos.y - localPos.y;

        float baseScale = 0.06f;
        float scaleFactor = baseScale / radarScale;

        float angle = -(localYaw - 90.0f) * (M_PI / 180.0f);
        float cosAngle = cos(angle);
        float sinAngle = sin(angle);

        Vector3D rotated;
        rotated.x = (delta.x * cosAngle - delta.y * sinAngle) * scaleFactor;
        rotated.y = -((delta.x * sinAngle + delta.y * cosAngle) * scaleFactor);

        float halfWidth = (radarSize.x / 2.0f) - 5.0f;
        float halfHeight = (radarSize.y / 2.0f) - 5.0f;
        
        rotated.x = std::clamp(rotated.x, -halfWidth, halfWidth);
        rotated.y = std::clamp(rotated.y, -halfHeight, halfHeight);

        Vector3D screen;
        screen.x = radarCenter.x + rotated.x;
        screen.y = radarCenter.y + rotated.y;
        
        return screen;
    }

    void renderRadar(ImDrawList* canvas) {
        ImGui::SetNextWindowSize(ImVec2(cl->MAP_RADAR_SIZE, cl->MAP_RADAR_SIZE));
        ImGui::SetNextWindowPos(ImVec2(cl->MAP_RADAR_POS_X, cl->MAP_RADAR_POS_Y));
        ImGui::Begin("Radar", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoBackground | 
            ImGuiWindowFlags_NoSavedSettings | 
            ImGuiWindowFlags_NoBringToFrontOnFocus);
        ImVec2 windowPos = ImGui::GetWindowPos();
        cl->MAP_RADAR_POS_X = windowPos.x;
        cl->MAP_RADAR_POS_Y = windowPos.y;

        ImVec2 drawPos = ImGui::GetCursorScreenPos();
        ImVec2 drawSize = ImGui::GetContentRegionAvail();
        ImVec2 midRadar = ImVec2(drawPos.x + (drawSize.x / 2), drawPos.y + (drawSize.y / 2));


        // Draw background
        canvas->AddRectFilled(
            ImVec2(drawPos.x, drawPos.y),
            ImVec2(drawPos.x + drawSize.x, drawPos.y + drawSize.y),
            ImColor(ImVec4(0.0f, 0.0f, 0.0f, cl->MAP_RADAR_BACKGROUND))
        );

        // Draw radar lines
        if (cl->MAP_RADAR_LINE) {
            ImVec2 startHorizontal(midRadar.x - drawSize.x / 2, midRadar.y);
            ImVec2 endHorizontal(midRadar.x + drawSize.x / 2, midRadar.y);
            ImVec2 startVertical(midRadar.x, midRadar.y - drawSize.y / 2);
            ImVec2 endVertical(midRadar.x, midRadar.y + drawSize.y / 2);

            canvas->AddLine(startHorizontal, endHorizontal, ImColor(1.0f, 1.0f, 1.0f, 0.5f));
            canvas->AddLine(startVertical, endVertical, ImColor(1.0f, 1.0f, 1.0f, 0.5f));
        }

        canvas->AddCircleFilled(
            midRadar,
            5,
            ImColor(0.0f, 1.0f, 0.0f, 1.0f)
        );

        for (int i = 0; i < players->size(); i++) {
            Player* p = players->at(i);
            if (!p->isEnemy || !p->isValid() || p->isLocal)
                continue;

            float radarDistance = util::inchesToMeters(p->distance2DToLocalPlayer);
            if (radarDistance >= 0.0f && radarDistance < cl->SENSE_MAX_RANGE) {
                Vector3D radarPos = WorldToRadar(
                    p->localOrigin,
                    lp->localOrigin,
                    cl->MAP_RADAR_ROTATE ? lp->viewAngles.y : 0.0f,
                    midRadar,
                    cl->MAP_RADAR_SCALE,
                    drawSize
                );

                ImVec2 enemyPos(radarPos.x, radarPos.y);
                
                float heightDiff = p->localOrigin.z - lp->localOrigin.z;
                
                ImColor dotColor;
                if (p->isDrone) {
                    dotColor = ImColor(0.0f, 0.0f, 0.99f, 0.99f); // Blue color for drones
                } else if (p->isItem) {
                    dotColor = ImColor(0.0f, 0.99f, 0.99f, 0.99f); // Cyan color for items
                } else {
                    dotColor = ImColor(0.99f, 0.0f, 0.0f, 0.99f); // Red color for enemies
                }

                canvas->AddCircleFilled(enemyPos, 5, dotColor);

                if (heightDiff < -50.0f) {
                    canvas->AddTriangleFilled(
                        ImVec2(enemyPos.x, enemyPos.y + 3),       // Bottom point
                        ImVec2(enemyPos.x - 3, enemyPos.y - 1),   // Top left
                        ImVec2(enemyPos.x + 3, enemyPos.y - 1),   // Top right
                        ImColor(1.0f, 1.0f, 1.0f, 0.99f)
                    );
                }
                else if (heightDiff > 50.0f) {
                    canvas->AddTriangleFilled(
                        ImVec2(enemyPos.x, enemyPos.y - 3),       // Top point
                        ImVec2(enemyPos.x - 3, enemyPos.y + 1),   // Bottom left
                        ImVec2(enemyPos.x + 3, enemyPos.y + 1),   // Bottom right
                        ImColor(1.0f, 1.0f, 1.0f, 0.99f)
                    );
                }

                if (p->isVisible) {
                    canvas->AddCircle(enemyPos, 7, ImColor(0.99f, 0.99f, 0.0f, 0.99f));
                }
            }
        }
        ImGui::End();
    }

    void renderSpectators(int totalSpectators, std::vector<std::string> spectators) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(ImVec2(0.0f, center.y), ImGuiCond_Once, ImVec2(0.02f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.3f);
        ImGui::Begin("Spectators", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoInputs);

        ImGui::Text("Spectators: ");
        ImGui::SameLine(); ImGui::TextColored(totalSpectators > 0 ? ImVec4(1, 0.343, 0.475, 1) : ImVec4(0.4, 1, 0.343, 1), "%d", totalSpectators);
        if (static_cast<int>(spectators.size()) > 0) {
            ImGui::Separator();
            for (int i = 0; i < static_cast<int>(spectators.size()); i++)
                ImGui::TextColored(ImVec4(1, 0.343, 0.475, 1), "> %s", spectators.at(i).c_str());
        }
        ImGui::End();
    }

    void renderMenu() {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(0.67f);
        ImGui::Begin("Menu", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoSavedSettings);

        if (ImGui::BeginTabBar("Categories")) {
            if (ImGui::BeginTabItem("About")) {
                ImGui::Text("GUI Made by andrazakii");
                ImGui::Checkbox("Aimbot", &cl->FEATURE_AIMBOT_ON);
                ImGui::Checkbox("Triggerbot", &cl->FEATURE_TRIGGERBOT_ON);
                ImGui::Checkbox("Recoil Control", &cl->FEATURE_RECOIL_ON);
                ImGui::Checkbox("Spectators", &cl->FEATURE_SPECTATORS_ON);
                ImGui::Checkbox("Show Dead Spectators", &cl->FEATURE_SPECTATORS_SHOW_DEAD);
                ImGui::Checkbox("Super Glide", &cl->FEATURE_SUPER_GLIDE_ON);
                ImGui::Checkbox("Map Radar", &cl->FEATURE_MAP_RADAR_ON);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Aimbot")) {
                ImGui::Text("Aimbot Settings");
                ImGui::Checkbox("Aimbot", &cl->FEATURE_AIMBOT_ON);
                if (cl->FEATURE_AIMBOT_ON) {
                    ImGui::SliderInt("Aimbot Hz", &cl->AIMBOT_HZ, 0, 240);
                    ImGui::SliderInt("Aimbot Delay", &cl->AIMBOT_DELAY, 0, 100);
                    ImGui::SliderFloat("Aimbot Speed", &cl->AIMBOT_SPEED, 0.0f, 100.0f);
                    ImGui::SliderFloat("Aimbot Smooth", &cl->AIMBOT_SMOOTH, 0.0f, 100.0f);
                    ImGui::SliderFloat("Aimbot Smooth Extra by Distance", &cl->AIMBOT_SMOOTH_EXTRA_BY_DISTANCE, 0.0f, 5000.0f);
                    ImGui::SliderFloat("Aimbot FOV", &cl->AIMBOT_FOV, 0.0f, 180.0f);
                    ImGui::SliderFloat("Aimbot FOV Extra by Zoom", &cl->AIMBOT_FOV_EXTRA_BY_ZOOM, 0.0f, 10.0f);
                    ImGui::SliderFloat("Aimbot Fast Area", &cl->AIMBOT_FAST_AREA, 0.0f, 1.0f);
                    ImGui::SliderFloat("Aimbot Slow Area", &cl->AIMBOT_SLOW_AREA, 0.0f, 1.0f);
                    ImGui::SliderFloat("Aimbot Weaken", &cl->AIMBOT_WEAKEN, 0.0f, 10.0f);
                    ImGui::Checkbox("Aimbot Spectators Weaken", &cl->AIMBOT_SPECTATORS_WEAKEN);
                    ImGui::Checkbox("Aimbot Predict Bullet Drop", &cl->AIMBOT_PREDICT_BULLETDROP);
                    ImGui::Checkbox("Aimbot Predict Movement", &cl->AIMBOT_PREDICT_MOVEMENT);
                    ImGui::Checkbox("Aimbot Allow Head Target", &cl->AIMBOT_HITBOX_HEAD);
                    ImGui::Checkbox("Aimbot Friendly Fire", &cl->AIMBOT_FRIENDLY_FIRE);
                    ImGui::Checkbox("Aimbot Legacy Mode", &cl->AIMBOT_LEGACY_MODE);
                    ImGui::SliderInt("Aimbot Max Distance", &cl->AIMBOT_MAX_DISTANCE, 0, 1000);
                    ImGui::SliderInt("Aimbot Min Distance", &cl->AIMBOT_MIN_DISTANCE, 0, 1000);
                    ImGui::SliderInt("Aimbot Zoomed Max Move", &cl->AIMBOT_ZOOMED_MAX_MOVE, 0, 100);
                    ImGui::SliderInt("Aimbot Hipfire Max Move", &cl->AIMBOT_HIPFIRE_MAX_MOVE, 0, 100);
                    ImGui::SliderInt("Aimbot Max Delta", &cl->AIMBOT_MAX_DELTA, 0, 100);
                    ImGui::Checkbox("Aimbot Activated by ADS", &cl->AIMBOT_ACTIVATED_BY_ADS);
                    ImGui::Checkbox("Aimbot Activated by Mouse", &cl->AIMBOT_ACTIVATED_BY_MOUSE);
                    ImGui::Checkbox("Aimbot Activated by Key", &cl->AIMBOT_ACTIVATED_BY_KEY);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Recoil")) {
                ImGui::Text("Recoil Settings");
                ImGui::Checkbox("Recoil Control", &cl->FEATURE_RECOIL_ON);
                if (cl->FEATURE_RECOIL_ON) {
                    ImGui::SliderFloat("Jitter X Amount", &cl->JITTER_X_AMOUNT, 0.0f, 10.0f);
                    ImGui::SliderFloat("Jitter Y Amount", &cl->JITTER_Y_AMOUNT, 0.0f, 5.0f);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Sense")) {
                ImGui::Text("Sense Settings");
                ImGui::SliderInt("Max Range", &cl->SENSE_MAX_RANGE, 0, 1000);
                // ImGui::Checkbox("360 Alert", &cl->SENSE_360_ALERT);
                ImGui::Checkbox("Show Box", &cl->SENSE_SHOW_BOX);
                ImGui::Checkbox("Show Player Bars", &cl->SENSE_SHOW_PLAYER_BARS);
                ImGui::Checkbox("Show Player Distances", &cl->SENSE_SHOW_PLAYER_DISTANCES);
                ImGui::Checkbox("Show Player Names", &cl->SENSE_SHOW_PLAYER_NAMES);
                ImGui::Checkbox("Show Player Levels", &cl->SENSE_SHOW_PLAYER_LEVELS);
                ImGui::Checkbox("Text Bottom", &cl->SENSE_TEXT_BOTTOM);
                ImGui::Checkbox("Show Dead", &cl->SENSE_SHOW_DEAD);
                ImGui::Checkbox("Show FOV", &cl->SENSE_SHOW_FOV);
                if (cl->SENSE_SHOW_FOV) {
                    ImGui::Checkbox("Show FOV when Aiming Only", &cl->SENSE_SHOW_FOV_AIM);
                }
                ImGui::Checkbox("Show Target", &cl->SENSE_SHOW_TARGET);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Radar")) {
                ImGui::Text("Radar Settings");
                ImGui::Checkbox("Radar", &cl->FEATURE_MAP_RADAR_ON);
                if (cl->FEATURE_MAP_RADAR_ON) {
                    ImGui::Checkbox("Map Radar Rotate", &cl->MAP_RADAR_ROTATE);
                    ImGui::Checkbox("Map Radar Line", &cl->MAP_RADAR_LINE);
                    ImGui::SliderFloat("Map Radar Background", &cl->MAP_RADAR_BACKGROUND, 0, 1);
                    ImGui::SliderFloat("Map Radar Scale", &cl->MAP_RADAR_SCALE, 0.1, 10);
                    ImGui::SliderInt("Map Radar Size", &cl->MAP_RADAR_SIZE, 0, 500);
                    ImGui::SliderFloat("Map Radar Position X", &cl->MAP_RADAR_POS_X, 0.0f, 500.0f);
                    ImGui::SliderFloat("Map Radar Position Y", &cl->MAP_RADAR_POS_Y, 0.0f, 500.0f);
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Items")) {
                ImGui::Text("Items");
                ImGui::BeginTable("Rows", 5);
                for (int i = 0; i < static_cast<int>(sizeof data::items / sizeof data::items[0]); i++) {
                    ImGui::TableNextColumn();
                    std::string id = data::items[i][0];
                    if (ImGui::RadioButton(id.c_str(), &data::selectedRadio, i)) printf("%d\n", data::selectedRadio);
                }
                ImGui::EndTable();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                ImGui::Text("Settings");

                // Add Save Button
                if (ImGui::Button("Save Settings")) {
                    saveSettings();
                    settingsSaved = true;
                }

                if (settingsSaved) {
                    ImGui::Text("Settings saved to nika.ini");
                }

                if (ImGui::Button("Unload")) {
                    shouldUnload = true;
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void saveSettings() {
        std::ofstream configFile("nika.ini");

        if (configFile.is_open()) {
            configFile << "# ENABLE/DISABLE FEATURES\n";
            configFile << "FEATURE_AIMBOT_ON                      " << (cl->FEATURE_AIMBOT_ON ? "YES" : "NO") << "   #[YES,NO] Does your aim suck?\n";
            configFile << "FEATURE_RECOIL_ON                      " << (cl->FEATURE_RECOIL_ON ? "YES" : "NO") << "   #[YES,NO] Cant control recoil?\n";
            configFile << "FEATURE_TRIGGERBOT_ON                  " << (cl->FEATURE_TRIGGERBOT_ON ? "YES" : "NO") << "   #[YES,NO] Do you pull instead of squeeze?\n";
            configFile << "FEATURE_SPECTATORS_ON                  " << (cl->FEATURE_SPECTATORS_ON ? "YES" : "NO") << "   #[YES,NO] Show spectators list\n";
            configFile << "FEATURE_SPECTATORS_SHOW_DEAD           " << (cl->FEATURE_SPECTATORS_SHOW_DEAD ? "YES" : "NO") << "   #[YES,NO] Also show dead players still connected\n";
            configFile << "FEATURE_SUPER_GLIDE_ON                 " << (cl->FEATURE_SUPER_GLIDE_ON ? "YES" : "NO") << "   #[YES,NO] Allow super glide activation?\n";
            configFile << "FEATURE_MAP_RADAR_ON                   " << (cl->FEATURE_MAP_RADAR_ON ? "YES" : "NO") << "   #[YES,NO] Show minimap radar\n";
            
            configFile << "\n# MAP RADAR\n";
            configFile << "MAP_RADAR_ROTATE                       " << (cl->MAP_RADAR_ROTATE ? "YES" : "NO") << "   #[YES,NO] Rotate the radar based on the player's yaw\n";
            configFile << "MAP_RADAR_LINE                         " << (cl->MAP_RADAR_LINE ? "YES" : "NO") << "   #[YES,NO] Draw a line pointing in the direction of each player's aim\n";
            configFile << "MAP_RADAR_BACKGROUND                   " << cl->MAP_RADAR_BACKGROUND << "   #[0-1] 0 = Transparent, 1 = Opaque\n";
            configFile << "MAP_RADAR_SCALE                        " << cl->MAP_RADAR_SCALE << "   #[0-3] 0 = 1:1, 1 = 1:2, 2 = 1:4, 3 = 1:8\n";
            configFile << "MAP_RADAR_SIZE                         " << cl->MAP_RADAR_SIZE << "   #[0-500] Size of the radar window\n";
            configFile << "MAP_RADAR_POS_X                        " << cl->MAP_RADAR_POS_X << "   #[] 1920*1080 X_215 Y_215\n";
            configFile << "MAP_RADAR_POS_Y                        " << cl->MAP_RADAR_POS_Y << "   #[] 2560*1440 X_335 Y_335\n";

            configFile << "\n# SENSE\n";
            configFile << "SENSE_VERBOSE                          " << cl->SENSE_VERBOSE << "    #[1-2] 2 = use Overlay, 1 = use CLI (text only)\n";
            configFile << "SENSE_MAX_RANGE                        " << cl->SENSE_MAX_RANGE << "  #[0-99999] Sense will not work if the target is too far. Units are meters\n";
            // configFile << "SENSE_360_ALERT                        " << (cl->SENSE_360_ALERT ? "YES" : "NO") << "  #[YES,NO] Alert when someone is behind you\n";
            configFile << "SENSE_SHOW_BOX                         " << (cl->SENSE_SHOW_BOX ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_PLAYER_BARS                 " << (cl->SENSE_SHOW_PLAYER_BARS ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_PLAYER_DISTANCES            " << (cl->SENSE_SHOW_PLAYER_DISTANCES ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_PLAYER_NAMES                " << (cl->SENSE_SHOW_PLAYER_NAMES ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_PLAYER_LEVELS               " << (cl->SENSE_SHOW_PLAYER_LEVELS ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_TEXT_BOTTOM                      " << (cl->SENSE_TEXT_BOTTOM ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_DEAD                        " << (cl->SENSE_SHOW_DEAD ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "SENSE_SHOW_FOV                         " << (cl->SENSE_SHOW_FOV ? "YES" : "NO") << "  #[YES,NO] Draw FOV circle\n";
            configFile << "SENSE_SHOW_TARGET                      " << (cl->SENSE_SHOW_TARGET ? "YES" : "NO") << "  #[YES,NO] Draw target line\n";

            configFile << "\n# TRIGGERBOT\n";
            configFile << "TRIGGERBOT_HIPFIRE_RANGE               " << cl->TRIGGERBOT_HIPFIRE_RANGE << "   #[0-99999] Max range triggerbot will work at when firing gun from hip\n";

            configFile << "\n# JITTER RECOIL\n";
            configFile << "JITTER_X_AMOUNT                         " << cl->JITTER_X_AMOUNT << "   #[0-100] Recoil jitter amount\n";
            configFile << "JITTER_Y_AMOUNT                         " << cl->JITTER_Y_AMOUNT << "   #[0-100] Recoil jitter amount\n";

            configFile << "\n# AIMBOT\n";
            configFile << "AIMBOT_HZ                              " << cl->AIMBOT_HZ << "  #[60-240] Aimbot cycles per second. Above 60Hz only used for MASTIFF + PEACEKEEPER triggerbot\n";
            configFile << "AIMBOT_DELAY                           " << cl->AIMBOT_DELAY << "    #[1-10] Mouse will move every N cycles\n";
            configFile << "AIMBOT_SPEED                           " << cl->AIMBOT_SPEED << "   #[1-999.9] Bigger = Faster (when SMOOTH+SMOOTH_EXTRA are balanced, you can adjust SPEED for both)\n";
            configFile << "AIMBOT_SMOOTH                          " << cl->AIMBOT_SMOOTH << "   #[1-999.9] Smaller = Faster\n";
            configFile << "AIMBOT_SMOOTH_EXTRA_BY_DISTANCE        " << cl->AIMBOT_SMOOTH_EXTRA_BY_DISTANCE << " #[1-99999] The closer the enemy the more smoothing. Legacy mode\n";
            configFile << "AIMBOT_FOV                             " << cl->AIMBOT_FOV << "  #[1-180.0] How close to the crosshair will the aimbot activate\n";
            configFile << "AIMBOT_FOV_EXTRA_BY_ZOOM               " << cl->AIMBOT_FOV_EXTRA_BY_ZOOM << "  #[0-10.0]\n";
            configFile << "AIMBOT_FAST_AREA                       " << cl->AIMBOT_FAST_AREA << " #[0.50-0.75] % of FOV area to allow more flick. Outer FOV\n";
            configFile << "AIMBOT_SLOW_AREA                       " << cl->AIMBOT_SLOW_AREA << " #[0.25-0.50] % of FOV area to allow less flick. Inner FOV\n";
            configFile << "AIMBOT_WEAKEN                          " << cl->AIMBOT_WEAKEN << "    #[2-10.0] AIMBOT_SPEED reduction by this divisor, toggle this with CURSOR_LEFT\n";

            configFile << "AIMBOT_SPECTATORS_WEAKEN               " << (cl->AIMBOT_SPECTATORS_WEAKEN ? "YES" : "NO") << "  #[YES,NO]\n";
            configFile << "AIMBOT_PREDICT_BULLETDROP              " << (cl->AIMBOT_PREDICT_BULLETDROP ? "YES" : "NO") << "  #[YES,NO] Self explanatory\n";
            configFile << "AIMBOT_PREDICT_MOVEMENT                " << (cl->AIMBOT_PREDICT_MOVEMENT ? "YES" : "NO") << "  #[YES,NO] Self explanatory\n";
            configFile << "AIMBOT_HITBOX_HEAD                     " << (cl->AIMBOT_HITBOX_HEAD ? "YES" : "NO") << "  #[YES,NO] Allow targeting head?\n";
            configFile << "AIMBOT_FRIENDLY_FIRE                   " << (cl->AIMBOT_FRIENDLY_FIRE ? "YES" : "NO") << "  #[YES,NO] Self explanatory\n";
            configFile << "AIMBOT_LEGACY_MODE                     " << (cl->AIMBOT_LEGACY_MODE ? "YES" : "NO") << "  #[YES,NO] Self explanatory\n";

            configFile << "AIMBOT_MAX_DISTANCE                    " << cl->AIMBOT_MAX_DISTANCE << "  #[0-99999] Aimbot will not work if the target is too far. Units are meters\n";
            configFile << "AIMBOT_MIN_DISTANCE                    " << cl->AIMBOT_MIN_DISTANCE << "    #[0-99999] Aimbot will not work if the target is too close. Units are meters\n";
            configFile << "AIMBOT_ZOOMED_MAX_MOVE                 " << cl->AIMBOT_ZOOMED_MAX_MOVE << "   #[1-99999] Self explanatory\n";
            configFile << "AIMBOT_HIPFIRE_MAX_MOVE                " << cl->AIMBOT_HIPFIRE_MAX_MOVE << "   #[1-99999] Self explanatory\n";
            configFile << "AIMBOT_MAX_DELTA                       " << cl->AIMBOT_MAX_DELTA << "   #[1-99999] Maximum speed increase/decrease\n";

            configFile << "AIMBOT_ACTIVATED_BY_ADS                " << (cl->AIMBOT_ACTIVATED_BY_ADS ? "YES" : "NO") << "  #[YES,NO] Aimbot will activate when zooming with a weapon\n";
            configFile << "AIMBOT_ACTIVATED_BY_MOUSE              " << (cl->AIMBOT_ACTIVATED_BY_MOUSE ? "YES" : "NO") << "  #[YES,NO] Aimbot will be activated when pressing the left mouse button\n";
            configFile << "AIMBOT_ACTIVATED_BY_KEY                " << (cl->AIMBOT_ACTIVATED_BY_KEY ? "YES" : "NO") << "  #[YES,NO] Aimbot will be activated when pressing the key below\n";

            configFile << "\n# KEYS\n";
            configFile << "AIMBOT_ACTIVATION_KEY                  " << cl->AIMBOT_ACTIVATION_KEY << "   #[Check key_codes.txt else leave empty or put NONE] Aimbot will be activated when this key is pressed\n";
            configFile << "AIMBOT_FIRING_KEY                      " << cl->AIMBOT_FIRING_KEY << "         #[Check key_codes.txt] Aimbot will press this key when it needs to fire\n";
            configFile << "SUPER_GLIDE_ACTIVATION_KEY             " << cl->SUPER_GLIDE_ACTIVATION_KEY << " #[Check key_codes.txt else leave empty or put NONE] Super glide will activate while this key is pressed\n";

            configFile.close();
        }
    }
};
