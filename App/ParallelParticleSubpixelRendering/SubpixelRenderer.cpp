#include "SubpixelRenderer.h"
#include <kvs/ParticleBuffer>
#include <kvs/PointObject>
#include <kvs/Camera>
#include <kvs/Assert>
#include <ParallelImageComposition/Lib/ImageCompositor.h>
#include <kvs/Timer>

namespace local
{
  /*==========================================================================*/
  /**
   *  Constructor.
   */
  /*==========================================================================*/
  SubpixelRenderer::SubpixelRenderer(const int rank, const int nnodes ):
    m_ref_point( NULL ),
    m_enable_rendering( true ),
    m_subpixel_level( 1 ),
    m_buffer( NULL ),
    m_rank( rank ),
    m_nnodes( nnodes ),
    m_composition_time( 0.0f )
  {
    BaseClass::setShader( kvs::Shader::Lambert() );
  }

  /*==========================================================================*/
  /**
   *  Constructor.
   *  @param point [in] pointer to the point object
   *  @param subpixel_level [in] subpixel level
   */
  /*==========================================================================*/
  SubpixelRenderer::SubpixelRenderer(
     const int rank,
     const int nnodes,
     const kvs::PointObject* point,
     const size_t subpixel_level ):
    m_ref_point( NULL ),
    m_enable_rendering( true ),
    m_subpixel_level( 1 ),
    m_buffer( NULL ),
    m_rank( rank ),
    m_nnodes( nnodes ),
    m_composition_time( 0.0f )
  {
    BaseClass::setShader( kvs::Shader::Lambert() );
    this->setSubpixelLevel( subpixel_level );
    this->attachPointObject( point );
  }

  /*==========================================================================*/
  /**
   *  Destructor.
   */
  /*==========================================================================*/
  SubpixelRenderer::~SubpixelRenderer()
  {
    this->deleteParticleBuffer();
  }

  /*==========================================================================*/
  /**
   *  Rendering.
   *  @param object [in] pointer to the object (PointObject)
   *  @param camera [in] pointet to the camera
   *  @param light [in] pointer to the light
   */
  /*==========================================================================*/
  void SubpixelRenderer::exec(
				   kvs::ObjectBase* object,
				   kvs::Camera* camera,
				   kvs::Light* light )
  {
    if ( !m_enable_rendering ) return;

    kvs::PointObject* point = kvs::PointObject::DownCast( object );
    if ( !m_ref_point ) this->attachPointObject( point );
    if ( point->normals().size() == 0 ) BaseClass::disableShading();

    BaseClass::startTimer();
    {
      this->create_image( point, camera, light );
      BaseClass::drawImage();
      this->cleanParticleBuffer();
    }
    BaseClass::stopTimer();
  }

  /*==========================================================================*/
  /**
   *  Create the point buffer.
   */
  /*==========================================================================*/
  bool SubpixelRenderer::createParticleBuffer(
						   const size_t width,
						   const size_t height,
						   const size_t subpixel_level )
  {
    m_buffer = new kvs::ParticleBuffer( width, height, subpixel_level );
    if ( !m_buffer ) return( false );

    return( true );
  }

  /*==========================================================================*/
  /**
   *  Clean the particle buffer.
   */
  /*==========================================================================*/
  void SubpixelRenderer::cleanParticleBuffer()
  {
    m_buffer->clean();
  }

  /*==========================================================================*/
  /**
   *  Delete the particle buffer.
   */
  /*==========================================================================*/
  void SubpixelRenderer::deleteParticleBuffer()
  {
    if ( m_buffer ) { delete m_buffer; m_buffer = NULL; }
  }

  /*==========================================================================*/
  /**
   *  Create the rendering image.
   *  @param point [in] pointer to the point object
   *  @param camera [in] pointer to the camera
   *  @param light [in] pointer to the light
   */
  /*==========================================================================*/
  void SubpixelRenderer::create_image(
					   const kvs::PointObject* point,
					   const kvs::Camera* camera,
					   const kvs::Light* light )
  {
    // Current rendering window size.
    const size_t current_width = BaseClass::windowWidth();
    const size_t current_height = BaseClass::windowHeight();

    // Updated rendering window size
    const size_t width = camera->windowWidth();
    const size_t height = camera->windowHeight();

    // Create memory region for the buffers, if the screen size is changed.
    if ( ( current_width != width ) || ( current_height != height ) )
      {
	BaseClass::setWindowSize( width, height );
	BaseClass::allocateColorData( width * height * 4 );
	BaseClass::allocateDepthData( width * height );

        this->deleteParticleBuffer();
        this->createParticleBuffer( width, height, m_subpixel_level );
      }

    // Initialize the frame buffers.
    BaseClass::fillColorData( 0 );
    BaseClass::fillDepthData( 0 );

    

    this->project_particle( point, camera, light );
  }

  /*==========================================================================*/
  /**
   *  Project the particles.
   *  @param point [in] pointer to the point object
   *  @param camera [in] pointer to the camera
   *  @param light [in] pointer to the light
   */
  /*==========================================================================*/
  void SubpixelRenderer::project_particle(
					       const kvs::PointObject* point,
					       const kvs::Camera* camera,
					       const kvs::Light* light )
  {
    kvs::Xform pvm( camera->projectionMatrix() * camera->viewingMatrix() * point->modelingMatrix() );
    float t[16]; pvm.toArray( t );

    const size_t w = camera->windowWidth() / 2;
    const size_t h = camera->windowHeight() / 2;

    // Set shader initial parameters.
    BaseClass::shader().set( camera, light, point );

    // Attach the shader and the point object to the point buffer.
    m_buffer->attachShader( &BaseClass::shader() );
    m_buffer->attachPointObject( point );

    // Aliases.
    const size_t nv = point->numberOfVertices();
    const kvs::Real32* v  = point->coords().data();

    size_t index3 = 0;
    const size_t bounds_width = BaseClass::windowWidth() - 1;
    const size_t bounds_height = BaseClass::windowHeight() - 1;
    for ( size_t index = 0; index < nv; index++, index3 += 3 )
      {
        /* Calculate the projected point position in the window coordinate system.
         * Ex.) Camera::projectObjectToWindow().
         */
        float p_tmp[4] = {
	  v[index3]*t[0] + v[index3+1]*t[4] + v[index3+2]*t[ 8] + t[12],
	  v[index3]*t[1] + v[index3+1]*t[5] + v[index3+2]*t[ 9] + t[13],
	  v[index3]*t[2] + v[index3+1]*t[6] + v[index3+2]*t[10] + t[14],
	  v[index3]*t[3] + v[index3+1]*t[7] + v[index3+2]*t[11] + t[15] };
        p_tmp[3] = 1.0f / p_tmp[3];
        p_tmp[0] *= p_tmp[3];
        p_tmp[1] *= p_tmp[3];
        p_tmp[2] *= p_tmp[3];

        const float p_win_x = ( 1.0f + p_tmp[0] ) * w;
        const float p_win_y = ( 1.0f + p_tmp[1] ) * h;
        const float depth   = ( 1.0f + p_tmp[2] ) * 0.5f;

        // Store the projected point in the point buffer.
        if ( ( 0 < p_win_x ) & ( 0 < p_win_y ) )
	  {
            if ( ( p_win_x < bounds_width ) & ( p_win_y < bounds_height ) )
	      {
                m_buffer->add( p_win_x, p_win_y, depth, index );
	      }
	  }
      }

    // Shading calculation.
    if ( BaseClass::isEnabledShading() ) m_buffer->enableShading();
    else m_buffer->disableShading();

    const size_t buffer_size = m_buffer->width() * m_buffer->height() * m_buffer->subpixelLevel()*m_buffer->subpixelLevel() ;
    const bool depth_testing = true;   
    kvs::ValueArray<kvs::UInt8> color_buffer = get_color(point);
    const kvs::ValueArray<kvs::Real32>& tmp_depth_buffer = static_cast<const kvs::ParticleBuffer*>(m_buffer)->depthBuffer();
    kvs::ValueArray<kvs::UInt8> pixels( buffer_size * 3 );
    kvs::ValueArray<kvs::Real32> depth_buffer(tmp_depth_buffer.size());
    
    for(size_t j = 0; j < tmp_depth_buffer.size(); j++)
      {
	if(tmp_depth_buffer[j] ==0)
	  depth_buffer[j] = 1;
	else
	  depth_buffer[j] = tmp_depth_buffer[j];
      }
    
    ParallelImageComposition::ImageCompositor compositor( m_rank, m_nnodes, MPI_COMM_WORLD );
    const int extended_width = m_buffer->width()  * m_buffer->subpixelLevel();
    const int extended_height = m_buffer->height() * m_buffer->subpixelLevel(); 

    compositor.initialize( extended_width, extended_height, depth_testing );
    kvs::Timer composition_timer( kvs::Timer::Start );
    compositor.run( color_buffer, depth_buffer );
    composition_timer.stop();
    m_composition_time = composition_timer.sec();
    this->createImage( &BaseClass::colorData(), &BaseClass::depthData(), color_buffer, depth_buffer );
  }

  kvs::ValueArray<kvs::UInt8> SubpixelRenderer::get_color(const kvs::PointObject* point) const 
  {
    kvs::ValueArray<kvs::UInt8> color_buffer( m_buffer->width() * m_buffer->height() * m_buffer->subpixelLevel()*m_buffer->subpixelLevel() * 4 );
    const kvs::UInt8* point_color = m_ref_point->colors().data();

    const kvs::ValueArray<kvs::UInt32>& index_buffer = static_cast<const kvs::ParticleBuffer*>(m_buffer)->indexBuffer();
    
    color_buffer.fill(255);
    for(size_t i =0 ; i < index_buffer.size() ; i++)
      {	
	if(m_buffer->depth(i) > 0.0f )
	  {
	    color_buffer[4*i+0] = static_cast<kvs::UInt8>(point_color[3*m_buffer->index(i) +0]);
	    color_buffer[4*i+1] = static_cast<kvs::UInt8>(point_color[3*m_buffer->index(i) +1]); 
	    color_buffer[4*i+2] = static_cast<kvs::UInt8>(point_color[3*m_buffer->index(i) +2]);
	    color_buffer[4*i+3] = 255; 
	  }
	else
	  color_buffer[4*i+3] = 0;
      }
    return color_buffer;
  }
  
  void SubpixelRenderer::createImage( kvs::ValueArray<kvs::UInt8>* color, kvs::ValueArray<kvs::Real32>* depth, kvs::ValueArray<kvs::UInt8> color_buffer, kvs::ValueArray<kvs::Real32> depth_buffer )
  {
    const float inv_ssize = 1.0f / ( m_buffer->subpixelLevel() * m_buffer->subpixelLevel() );
    const float normalize_alpha = 255.0f * inv_ssize;
    
    size_t pindex = 0;
    size_t pindex4 = 0;
    size_t by_start = 0;
    const size_t bw = m_buffer->width() * m_buffer->subpixelLevel();
   
    for( size_t py = 0; py < m_buffer->height(); py++, by_start += m_buffer->subpixelLevel() )
      {
        size_t bx_start = 0;
        for( size_t px = 0; px < m_buffer->width(); px++, pindex++, pindex4 += 4, bx_start += m_buffer->subpixelLevel() )
	  {
            float R = 0.0f;
            float G = 0.0f;
            float B = 0.0f;
            float D = 0.0f;
            size_t npoints = 0;
            for( size_t by = by_start; by < by_start + m_buffer->subpixelLevel(); by++ )
	      {
                const size_t bindex_start = bw * by;
                for( size_t bx = bx_start; bx < bx_start + m_buffer->subpixelLevel(); bx++ )
		  {
                    const size_t bindex = bindex_start + bx;
      		    if( depth_buffer[bindex] > 0.0f )
		      {
			R += color_buffer[ bindex * 4 + 0 ];
			G += color_buffer[ bindex * 4 + 1 ];
			B += color_buffer[ bindex * 4 + 2 ];
			//D = kvs::Math::Min( D, depth_buffer[ bindex ] );
			D = kvs::Math::Max( D, depth_buffer[ bindex ] );
			npoints++;		
		      }
		  }
	      }
	    
            R *= inv_ssize;
	    G *= inv_ssize;
            B *= inv_ssize;
	    
            (*color)[ pindex4 + 0 ] = static_cast<kvs::UInt8>(R);
            (*color)[ pindex4 + 1 ] = static_cast<kvs::UInt8>(G);
            (*color)[ pindex4 + 2 ] = static_cast<kvs::UInt8>(B);
            (*color)[ pindex4 + 3 ] = static_cast<kvs::UInt8>( npoints * normalize_alpha );
            (*depth)[ pindex ] = ( npoints == 0 ) ? 1.0f : D;
	  }
      }
  }

  

} // end of namespace local
