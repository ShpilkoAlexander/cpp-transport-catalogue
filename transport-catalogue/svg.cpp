#include "svg.h"
#include <algorithm>
#include <string>
#include <string_view>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}


Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    bool is_first = true;
    for (const Point& point : points_) {
        if (is_first) {
            out << point.x << "," << point.y;
            is_first = false;
            continue;
        }
        out << " " << point.x << "," << point.y;
    }
    out << "\"";
    RenderAttrs(context.out);
    out << "/>";
}

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    for (auto [ch, spec_ch] : spec_symbols_) {
        auto founded_ch = data.find(ch);
        while (founded_ch != std::string::npos) {
             data.replace(founded_ch, 1, spec_ch);
             founded_ch  = data.find(ch);
        }
    }

    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<text";
    RenderAttrs(out);
    out << " x=\"" << pos_.x << "\" y=\"" << pos_.y << "\" ";
    out << "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" ";
    out << "font-size=\"" << font_size_ <<"\"";

    if (font_family_ != "") {
        out << " font-family=\"" << font_family_ <<"\"";
    }
    if (font_weight_ != "") {
        out << " font-weight=\"" << font_weight_ <<"\"";
    }


    out << ">" << data_ << "</text>";
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

// Выводит в ostream svg-представление документа
void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;

    RenderContext contex(out, 2, 2);
    for (auto& obj : objects_) {
        obj->Render(contex);
    }
    out << "</svg>"sv << std::endl; //check endl
}

void OstreamColorPrinter::operator()(std::monostate) const {
    out << "none";
}
void OstreamColorPrinter::operator()(std::string color) const {
    out << color;
}
void OstreamColorPrinter::operator()(Rgb color) const {
    out << "rgb(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << ","
        << static_cast<int>(color.blue) << ")";
}
void OstreamColorPrinter::operator()(Rgba color) const {
   out << "rgba(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green)
       << "," << static_cast<int>(color.blue) << "," << color.opacity  <<")";
}

std::ostream& operator<<(std::ostream& out, Color color) {
    std::visit(OstreamColorPrinter{out}, color);
    return out;
}

}  // namespace svg
