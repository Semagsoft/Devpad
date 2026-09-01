/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoShader: custom QSG material for gradient selection / cursor glow.
 * Still QSG, true GLSL shader.
 */

#ifndef PRIMOSHADER_H
#define PRIMOSHADER_H

#include <QColor>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGVertexColorMaterial>

class PrimoGradientMaterial : public QSGMaterial
{
public:
    PrimoGradientMaterial();
    QSGMaterialType* type() const override;
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial* other) const override;

    QColor colorTop = QColor(0xc4, 0xa7, 0xe7, 180);
    QColor colorBottom = QColor(0x89, 0xb4, 0xfa, 180);
    float opacity = 1.0f;
};

class PrimoGradientShader : public QSGMaterialShader
{
public:
    PrimoGradientShader();
    bool updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* oldMaterial) override;
};

QSGGeometryNode* createGradientRectNode(const QRectF& rect, const QColor& top, const QColor& bottom, float opacity = 1.0f);

#endif // PRIMOSHADER_H
