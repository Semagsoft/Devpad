/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "primoshader.h"

#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QSGVertexColorMaterial>

PrimoGradientMaterial::PrimoGradientMaterial()
{
    setFlag(Blending, true);
}

QSGMaterialType* PrimoGradientMaterial::type() const
{
    static QSGMaterialType t;
    return &t;
}

QSGMaterialShader* PrimoGradientMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new PrimoGradientShader;
}

int PrimoGradientMaterial::compare(const QSGMaterial* other) const
{
    auto* o = static_cast<const PrimoGradientMaterial*>(other);
    if (colorTop.rgba() != o->colorTop.rgba()) return colorTop.rgba() < o->colorTop.rgba() ? -1 : 1;
    if (colorBottom.rgba() != o->colorBottom.rgba()) return colorBottom.rgba() < o->colorBottom.rgba() ? -1 : 1;
    if (!qFuzzyCompare(opacity, o->opacity)) return opacity < o->opacity ? -1 : 1;
    return 0;
}

PrimoGradientShader::PrimoGradientShader()
{
    // Use vertex-color shader (no external .qsb needed) – gradient via vertex colors
}

bool PrimoGradientShader::updateUniformData(RenderState& state, QSGMaterial* newMaterial, QSGMaterial* /*oldMaterial*/)
{
    Q_UNUSED(state); Q_UNUSED(newMaterial);
    return false;
}

QSGGeometryNode* createGradientRectNode(const QRectF& rect, const QColor& top, const QColor& bottom, float opacity)
{
    auto* node = new QSGGeometryNode;
    auto* geom = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 4);
    geom->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    QSGGeometry::ColoredPoint2D* v = geom->vertexDataAsColoredPoint2D();
    // Top two vertices = top color, bottom two = bottom color (vertical gradient)
    v[0].x = rect.x(); v[0].y = rect.y();
    v[0].r = top.red(); v[0].g = top.green(); v[0].b = top.blue(); v[0].a = int(top.alpha() * opacity);
    v[1].x = rect.x() + rect.width(); v[1].y = rect.y();
    v[1].r = top.red(); v[1].g = top.green(); v[1].b = top.blue(); v[1].a = int(top.alpha() * opacity);
    v[2].x = rect.x(); v[2].y = rect.y() + rect.height();
    v[2].r = bottom.red(); v[2].g = bottom.green(); v[2].b = bottom.blue(); v[2].a = int(bottom.alpha() * opacity);
    v[3].x = rect.x() + rect.width(); v[3].y = rect.y() + rect.height();
    v[3].r = bottom.red(); v[3].g = bottom.green(); v[3].b = bottom.blue(); v[3].a = int(bottom.alpha() * opacity);
    auto* mat = new QSGVertexColorMaterial;
    mat->setFlag(QSGMaterial::Blending, true);
    node->setGeometry(geom);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(mat);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}
