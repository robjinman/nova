#include <model.hpp>
#include <gtest/gtest.h>

class MeshTest : public testing::Test
{
  public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};

TEST_F(MeshTest, calcOffsetInVertex_first_attribute_returns_zero)
{
  std::vector<BufferUsage> layout{
    BufferUsage::AttrPosition,
    BufferUsage::AttrNormal,
    BufferUsage::AttrTexCoord
  };

  auto offset = calcOffsetInVertex(layout, BufferUsage::AttrPosition);
  EXPECT_EQ(0, offset);
}

TEST_F(MeshTest, createVertexArray_single_vertex_all_attributes)
{
  Mesh mesh{MeshFeatureSet{
    .vertexLayout = {
      BufferUsage::AttrPosition,
      BufferUsage::AttrNormal,
      BufferUsage::AttrTexCoord
    },
    .isInstanced = false,
    .isSkybox = false,
    .isAnimated = false
  }};
  mesh.attributeBuffers.push_back(createBuffer<Vec3f>({{ 1, 2, 3 }}, BufferUsage::AttrPosition));
  mesh.attributeBuffers.push_back(createBuffer<Vec3f>({{ 4, 5, 6 }}, BufferUsage::AttrNormal));
  mesh.attributeBuffers.push_back(createBuffer<Vec2f>({{ 7, 8 }}, BufferUsage::AttrTexCoord));
  auto vertexData = createVertexArray(mesh);

  struct Vertex
  {
    Vec3f pos;
    Vec3f normal;
    Vec2f texCoord;
  };
  auto vertices = fromBytes<Vertex>(vertexData);

  ASSERT_EQ(1, vertices.size());
  EXPECT_EQ(Vec3f({ 1, 2, 3 }), vertices[0].pos);
  EXPECT_EQ(Vec3f({ 4, 5, 6 }), vertices[0].normal);
  EXPECT_EQ(Vec2f({ 7, 8 }), vertices[0].texCoord);
}
