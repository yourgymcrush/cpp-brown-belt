#include <map>

#include "Common.h"

using namespace std;

// Этот файл сдаётся на проверку
// Здесь напишите реализацию необходимых классов-потомков `IShape`

class Shape : public IShape
{
private:
    ShapeType mType = ShapeType::Rectangle;

protected:
    std::shared_ptr<ITexture> m_texture;
    Size m_size;
    Point m_point;
    virtual bool isInShape(Point p) const = 0;

public:
    Shape(ShapeType type) : mType(type) {}

    virtual std::unique_ptr<IShape> Clone() const
    {
        auto clone = MakeShape(mType);

        clone->SetPosition(m_point);
        clone->SetSize(m_size);
        clone->SetTexture(m_texture);
        
        return std::move(clone);
    }

    virtual void SetPosition(Point point)
    {
        m_point = point;
    }
    virtual Point GetPosition() const
    {
        return m_point;
    }

    virtual void SetSize(Size size)
    {
        m_size = size;
    }
    virtual Size GetSize() const
    {
        return m_size;
    }

    virtual void SetTexture(std::shared_ptr<ITexture> texture)
    {
        m_texture = texture;
    }
    virtual ITexture* GetTexture() const
    {
        return m_texture.get();
    }

    virtual void Draw(Image& image) const
    {
        int textureX = 0;
        int textureY = 0;

        for(int imageY = m_point.y; imageY < std::min(m_point.y + m_size.height, static_cast<int>(image.size())); imageY++)
        {
            for(int imageX = m_point.x; imageX < std::min(m_point.x + m_size.width, static_cast<int>(image[0].size())); imageX++)
            {   
                textureY = imageY - m_point.y;
                textureX = imageX - m_point.x;
                if (isInShape({textureX, textureY}))
                {
                    image[imageY][imageX] = m_texture
                                            && textureY < m_texture->GetSize().height
                                            && textureX < m_texture->GetSize().width
                                            ? m_texture->GetImage()[textureY][textureX] : '.';
                }
            }
        }
    }
};

class Rectangle : public Shape
{
public:
    Rectangle() : Shape(ShapeType::Rectangle) {}
protected:
    virtual bool isInShape(Point p) const
    {
        (void) p;
        return true;
    }
};

class Ellipse : public Shape
{
public:
    Ellipse() : Shape(ShapeType::Ellipse) {}
protected:
    virtual bool isInShape(Point p) const
    {
        return IsPointInEllipse(p, m_size);
    }
};


unique_ptr<IShape> MakeShape(ShapeType shape_type) {
    switch (shape_type)
    {
    case ShapeType::Rectangle:
        return std::make_unique<Rectangle>();
    case ShapeType::Ellipse:
        return std::make_unique<Ellipse>();
    }
}