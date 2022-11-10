#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <queue>
#include <algorithm>

#include <kvs/Application>
#include <kvs/Screen>
#include <kvs/PointObject>
#include <kvs/LineObject>
#include <kvs/PolygonObject>
#include <kvs/RGBColor>
#include <kvs/ColorMap>
#include <kvs/EventListener>
#include <kvs/Slider>
#include <kvs/Label>
#include <kvs/String>
#include <kvs/ValueArray>

using namespace std;

const bool OPTIMUM_POSITION = true;
const bool HEATMAP = true;
const bool OUTPUT_DISTANCES = false;

const string optimum_file = "Output_optimum";

const vector<string> input_files = {
    "Output_interval_10",
    "Output_interval_30",
    "Output_interval_50"
};

/*
const vector<string> input_files = {
    "Output_slerp",
    "Output_squad"
};
*/

const vector<kvs::UInt8> path_colors = {
    255, 0, 0,
    0, 255, 0,
    0, 0, 255
};

/*
const vector<kvs::UInt8> path_colors = {
    255, 255, 0,
    0, 255, 0
};
*/

const vector<string> path_labels = {
    "interval: 10",
    "interval: 30",
    "interval: 50"
};

/*
const vector<string> path_labels = {
    "slerp",
    "squad"
};
*/

const vector<string> color_labels = {
    "Red",
    "Green",
    "Blue"
};

/*
const vector<string> color_labels = {
    "Yellow",
    "Green"
};
*/

tuple<vector<float>, float, float> loadTable( string file_path )
{
    vector<float> table;
    ifstream file( file_path );
    string line;

    while( getline( file, line ) )
    {
        stringstream line_ss( line );
        string elem;

        while( getline( line_ss, elem, ',' ) )
        {
            stringstream elem_ss( elem );
            float value;
            elem_ss >> value;
            table.push_back( value );
        }
    }

    float max_value = *max_element( table.begin(), table.end() );
    float min_value = *min_element( table.begin(), table.end() );
    
    return { table, max_value, min_value };
}

vector<float> loadPosition( string file_path )
{
    vector<float> positions;
    ifstream file( file_path );
    string line;
    
    getline( file, line );
    while( getline( file, line ) )
    {
        stringstream line_ss( line );
        string elem;

        getline( line_ss, elem, ',' );
        while( getline( line_ss, elem, ',' ) )
        {
            stringstream elem_ss( elem );
            float value;
            elem_ss >> value;
            positions.push_back( value );
        }
    }

    return positions;
}

kvs::LineObject* createAxis()
{
    kvs::LineObject* object = new kvs::LineObject();

    kvs::ValueArray<float> coords = {
        15.0, 0.0, 0.0, -15.0, 0.0, 0.0,
        0.0, 15.0, 0.0, 0.0, -15.0, 0.0,
        0.0, 0.0, 15.0, 0.0, 0.0, -15.0
    };

    kvs::ValueArray<kvs::UInt32> connections = {
        0, 1,
        2, 3,
        4, 5
    };

    object->setCoords( coords );
    object->setColor( kvs::RGBColor::Black() );
    object->setConnections( connections );
    object->setSize( 3 );
    object->setLineType( kvs::LineObject::Segment );
    object->setColorType( kvs::LineObject::LineColor );

    return object;
}

kvs::PolygonObject* createSphere( size_t dim_lati, size_t dim_long )
{
    kvs::PolygonObject* object = new kvs::PolygonObject();

    vector<float> coords = { 0.0, 12.0 * 0.99, 0.0 };
    vector<kvs::UInt8> colors;
    vector<float> normals = { 0.0, 1.0, 0.0 };
    vector<kvs::UInt32> connections;

    float dt = kvs::Math::pi / ( dim_lati - 1 );
    float dp = 2.0 * kvs::Math::pi / dim_long;

    
    for( size_t i = 0; i < dim_lati - 1; i++ )
    {
        for( size_t j = 0; j < dim_long; j++ )
        {
            float theta = dt * ( i + 0.5 );
            float phi = dp * ( j - 0.5 );
            float x = 12.0 * sin( theta ) * sin( phi );
            float y = 12.0 * cos( theta );
            float z = 12.0 * sin( theta ) * cos( phi );
            coords.push_back( x * 0.99 );
            coords.push_back( y * 0.99 );
            coords.push_back( z * 0.99 );
            normals.push_back( x / 12.0 );
            normals.push_back( y / 12.0 );
            normals.push_back( z / 12.0 );
        }
    }

    coords.push_back( 0.0 );
    coords.push_back( -12.0 );
    coords.push_back( 0.0 );
    normals.push_back( 0.0 );
    normals.push_back( -1.0 );
    normals.push_back( 0.0 );

    for( size_t i = 0; i < dim_lati; i++ )
    {
        for( size_t j = 0; j < dim_long; j++ )
        {
            if( i == 0 )
            {
                connections.push_back( 0 );
                connections.push_back( j + 1 );
                connections.push_back( ( j + 1 ) % dim_long + 1 );
                connections.push_back( 0 );
            }
            else if( i == dim_lati - 1 )
            {
                connections.push_back( ( dim_lati - 2 ) * dim_long + j + 1 );
                connections.push_back( ( dim_lati - 1 ) * dim_long + 1 );
                connections.push_back( ( dim_lati - 1 ) * dim_long + 1 );
                connections.push_back( ( dim_lati - 2 ) * dim_long + ( j + 1 ) % dim_long + 1 );
            }
            else
            {
                connections.push_back( ( i - 1 ) * dim_long + j + 1 );
                connections.push_back( i * dim_long + j + 1 );
                connections.push_back( i * dim_long + ( j + 1 ) % dim_long + 1 );
                connections.push_back( ( i - 1 ) * dim_long + ( j + 1 ) % dim_long + 1 );
            }
        }
    }

    for( size_t i = 0; i < dim_lati * dim_long * 3; i++ )
    {
        colors.push_back( 192 );
    }

    object->setCoords( kvs::ValueArray<float>( coords ) );
    object->setColors( kvs::ValueArray<kvs::UInt8>( colors ) );
    object->setNormals( kvs::ValueArray<float>( normals ) );
    object->setConnections( kvs::ValueArray<kvs::UInt32>( connections ) );
    object->setPolygonType( kvs::PolygonObject::Quadrangle );
    object->setColorType( kvs::PolygonObject::PolygonColor );
    object->setNormalType( kvs::PolygonObject::VertexNormal );

    return object;
}

kvs::PointObject* createPoints( vector<float>& position )
{
    const size_t number_of_points = 10;
    kvs::PointObject* points = new kvs::PointObject();

    vector<float> coords;

    for( size_t i = 0; i < number_of_points; i++ )
    {
        coords.push_back( position[0] );
        coords.push_back( position[1] );
        coords.push_back( position[2] );
    }

    vector<kvs::UInt8> colors;

    for( size_t i = 0; i < number_of_points; i++ )
    {
        for( size_t j = 0; j < 3; j++ )
        {
            colors.push_back( 255 - 255 * ( i + 1 ) / number_of_points );    
        }
    }

    points->setCoords( kvs::ValueArray<float>( coords ) );
    points->setColors( kvs::ValueArray<kvs::UInt8>( colors ) );
    points->setSize( 5 );

    return points;
}

vector<kvs::LineObject*> createLines( vector<vector<float>>& positions )
{
    const size_t number_of_lines = 10;
    vector<kvs::LineObject*> lines;

    for( size_t i = 0; i < number_of_lines; i++ )
    {
        vector<float> coords;
        vector<kvs::UInt8> colors;
        vector<kvs::UInt32> connections;

        for( size_t j = 0; j < input_files.size(); j++ )
        {
            float x_start = positions[j][0];
            float y_start = positions[j][1];
            float z_start = positions[j][2];
            float x_end = positions[j][0];
            float y_end = positions[j][1];
            float z_end = positions[j][2];
            coords.push_back( x_start );
            coords.push_back( y_start );
            coords.push_back( z_start );
            coords.push_back( x_end );
            coords.push_back( y_end );
            coords.push_back( z_end );

            kvs::UInt8 r_start = max( path_colors[j*3], static_cast<kvs::UInt8>( 255 - 255 * i / number_of_lines ) );
            kvs::UInt8 g_start = max( path_colors[j*3+1], static_cast<kvs::UInt8>( 255 - 255 * i / number_of_lines ) );
            kvs::UInt8 b_start = max( path_colors[j*3+2], static_cast<kvs::UInt8>( 255 - 255 * i / number_of_lines ) );
            kvs::UInt8 r_end = max( path_colors[j*3], static_cast<kvs::UInt8>( 255 - 255 * ( i + 1 ) / number_of_lines ) );
            kvs::UInt8 g_end = max( path_colors[j*3+1], static_cast<kvs::UInt8>( 255 - 255 * ( i + 1 ) / number_of_lines ) );
            kvs::UInt8 b_end = max( path_colors[j*3+2], static_cast<kvs::UInt8>( 255 - 255 * ( i + 1 ) / number_of_lines ) );
            colors.push_back( r_start );
            colors.push_back( g_start );
            colors.push_back( b_start );
            colors.push_back( r_end );
            colors.push_back( g_end );
            colors.push_back( b_end );

            connections.push_back( j * 2 );
            connections.push_back( j * 2 + 1 );
        }

        kvs::LineObject* line = new kvs::LineObject();
        line->setCoords( kvs::ValueArray<float>( coords ) );
        line->setColors( kvs::ValueArray<kvs::UInt8>( colors ) );
        line->setConnections( kvs::ValueArray<kvs::UInt32>( connections ) );
        line->setSize( 2 );
        line->setLineType( kvs::LineObject::Segment );
        line->setColorType( kvs::LineObject::VertexColor );

        lines.push_back( line );
    }

    return lines;
}

kvs::ValueArray<kvs::UInt8> createHeatmapColors( vector<float>& table, kvs::ColorMap& cmap )
{
    vector<kvs::UInt8> colors;

    for( size_t i = 0; i < table.size(); i++ )
    {
        kvs::RGBColor color = cmap.at( table[i] );
        colors.push_back( color.r() );
        colors.push_back( color.g() );
        colors.push_back( color.b() );
    }

    return kvs::ValueArray<kvs::UInt8>( colors );
}

int main( int argc, char** argv )
{
    vector<vector<float>> tables;
    vector<float> max_values;
    vector<float> min_values;
    vector<float> optimum_position;
    vector<vector<float>> positions;
    kvs::ColorMap cmap = kvs::ColorMap::CoolWarm( 256 );

    const size_t time_steps = 1500;
    const size_t time_interval = 10;
    const size_t dim_lati = 25;
    const size_t dim_long = 50;

    if( OPTIMUM_POSITION )
    {
        string input_file_path = "./Outputs/" + optimum_file + "/output_path_positions.csv";
        optimum_position = loadPosition( input_file_path );

        if( HEATMAP )
        {
            for( size_t i = 0; i < time_steps; i++ )
            {
                size_t time_step = i * time_interval;
                string input_file_name =  "output_entropy_table_" + kvs::String::From( time_step, 6, '0' ) + ".csv";
                string input_file_path = "./Outputs/" + optimum_file + "/" + input_file_name;
                auto [a, b, c] = loadTable( input_file_path );
                tables.push_back( a );
                max_values.push_back( b );
                min_values.push_back( c );
            }
            
            float max_value = *max_element( max_values.begin(), max_values.end() );
            float min_value = *min_element( min_values.begin(), min_values.end() );
            cmap.setRange( min_value, max_value );
        }
    }
    
    for( size_t i = 0; i < input_files.size(); i++ )
    {
        string input_file_path = "./Outputs/" + input_files[i] + "/output_path_positions.csv";
        auto p = loadPosition( input_file_path );
        positions.push_back( p );
    }

    if( OPTIMUM_POSITION && OUTPUT_DISTANCES )
    {
        for( size_t i = 0; i < input_files.size(); i++ )
        {
            string output_file_name = "./Outputs/" + input_files[i] + "/output_distances.csv";
            ofstream gaps( output_file_name );
            gaps << "Time,Distance" << endl;

            for( size_t j = 0; j < time_steps; j++ )
            {
                vector<float> p0 = { optimum_position[3*j], optimum_position[3*j+1], optimum_position[3*j+2] };
                vector<float> p = { positions[i][3*j], positions[i][3*j+1], positions[i][3*j+2] };
                float distance = sqrt( pow( p0[0] - p[0], 2.0 ) + pow( p0[1] - p[1], 2.0 ) + pow( p0[2] - p[2], 2.0 ) );

                gaps << j * time_interval << "," << distance << endl;
            }

            gaps.close();
        }
    }

    kvs::Application app( argc, argv );
    kvs::Screen screen( &app );
    screen.setTitle( "PathViewer" );
    screen.create();

    kvs::LineObject* axis = createAxis();
    kvs::PolygonObject* sphere = createSphere( dim_lati, dim_long );
    vector<kvs::LineObject*> lines = createLines( positions );

    screen.registerObject( axis );
    screen.registerObject( sphere );

    for( size_t i = 0; i < lines.size(); i++ )
    {
        screen.registerObject( lines[i] );
    }

    kvs::PointObject* points;

    if( OPTIMUM_POSITION )
    {
        points = createPoints( optimum_position );
        screen.registerObject( points );

        if( HEATMAP )
        {
            kvs::ValueArray<kvs::UInt8> colors = createHeatmapColors( tables[0], cmap );
            sphere->setColors( colors );
        }
    }

    kvs::Label label( &screen );
    kvs::Font font( kvs::Font::Sans, kvs::Font::Bold, 20 );

    for( size_t i = 0; i < input_files.size(); i++ )
    {
        stringstream text;
        text << path_labels[i] << " (" << color_labels[i] << ") ";
        label.addText( text.str() );
    }

    label.setFont( font );
    label.anchorToTopRight();
    label.show();

    bool playing = false;
    size_t time_index = 0;

    kvs::Slider slider( &screen );
    slider.anchorToBottomRight();
    slider.setCaption( "Frame: " + kvs::String::From( 0, 6, '0' ) );
    slider.setRange( 0, time_steps - 1 );
    slider.setValue( time_index );
    slider.sliderMoved( [&]()
    {
        auto index = kvs::Math::Round( slider.value() );
        auto frame = kvs::String::From( index, 6, '0' );
        time_index = index;
        slider.setValue( index );
        slider.setCaption( "Frame: " + frame );
    } );
    slider.sliderPressed( [&]()
    {
        playing = false;
    } );
    slider.sliderReleased( [&]()
    {
        auto index = kvs::Math::Round( slider.value() );
        auto frame = kvs::String::From( index, 6, '0' );
        time_index = index;
        slider.setValue( index );
        slider.setCaption( "Frame: " + frame );

        for( size_t i = 0; i < lines.size(); i++ )
        {
            size_t time_start = max( index - lines.size() + i, static_cast<size_t>( 0 ) );
            size_t time_end = max( index - lines.size() + i + 1, static_cast<size_t>( 0 ) );
            vector<float> coords;
            
            for( size_t j = 0; j < input_files.size(); j++ )
            {
                float x_start = positions[j][time_start*3];
                float y_start = positions[j][time_start*3+1];
                float z_start = positions[j][time_start*3+2];
                float x_end = positions[j][time_end*3];
                float y_end = positions[j][time_end*3+1];
                float z_end = positions[j][time_end*3+2];
                coords.push_back( x_start );
                coords.push_back( y_start );
                coords.push_back( z_start );
                coords.push_back( x_end );
                coords.push_back( y_end );
                coords.push_back( z_end );
            }

            lines[i]->setCoords( kvs::ValueArray<float>( coords ) );
        }

        if( OPTIMUM_POSITION )
        {
            vector<float> coords;

            for( size_t i = 0; i < points->numberOfVertices(); i++ )
            {
                size_t time_step = max( index - points->numberOfVertices() + i + 1, static_cast<size_t>( 0 ) );
                coords.push_back( optimum_position[time_step*3] );
                coords.push_back( optimum_position[time_step*3+1] );
                coords.push_back( optimum_position[time_step*3+2] );
            }

            points->setCoords( kvs::ValueArray<float>( coords ) );

            if( HEATMAP )
            {
                kvs::ValueArray<kvs::UInt8> colors = createHeatmapColors( tables[index], cmap );
                sphere->setColors( colors );
            }
        }

        screen.redraw();
    } );
    slider.show();

    const size_t fps = 20;
    const float delta_time = 1000.0 / fps;
    kvs::EventListener event;
    event.keyPressEvent( [&]( kvs::KeyEvent* e )
    {
        switch ( e->key() )
        {
            case kvs::Key::Space:
            {
                if ( playing ) { playing = false; }
                else if ( time_index == time_steps ) { time_index = 0; playing = true; }
                else { playing = true; }
                break;
            }
            default: break;
        }
    } );
    event.timerEvent( [&]( kvs::TimeEvent* e )
    {
        if ( playing )
        {
            time_index += 1;
            auto frame = kvs::String::From( time_index, 6, '0' );
            slider.setValue( time_index );
            slider.setCaption( "Frame: " + frame );

            if( time_index < time_steps )
            {
                for( size_t i = 0; i < lines.size(); i++ )
                {
                    if( i < lines.size() - 1 )
                    {
                        lines[i]->setCoords( lines[i+1]->coords() );
                    }
                    else
                    {
                        vector<float> coords;

                        for( size_t j = 0; j < input_files.size(); j++ )
                        {
                            float x_start = positions[j][time_index*3];
                            float y_start = positions[j][time_index*3+1];
                            float z_start = positions[j][time_index*3+2];
                            float x_end = positions[j][time_index*3+3];
                            float y_end = positions[j][time_index*3+4];
                            float z_end = positions[j][time_index*3+5];
                            coords.push_back( x_start );
                            coords.push_back( y_start );
                            coords.push_back( z_start );
                            coords.push_back( x_end );
                            coords.push_back( y_end );
                            coords.push_back( z_end );
                        }

                        lines[i]->setCoords( kvs::ValueArray<float>( coords ) );
                    }
                }

                if( OPTIMUM_POSITION )
                {
                    vector<float> coords;

                    for( size_t i = 0; i < points->numberOfVertices(); i++ )
                    {
                        if( i < points->numberOfVertices() - 1 )
                        {
                            auto p = points->coord( i + 1 );
                            coords.push_back( p.x() );
                            coords.push_back( p.y() );
                            coords.push_back( p.z() );
                        }
                        else
                        {
                            coords.push_back( optimum_position[time_index*3] );
                            coords.push_back( optimum_position[time_index*3+1] );
                            coords.push_back( optimum_position[time_index*3+2] );
                        }
                    }

                    points->setCoords( kvs::ValueArray<float>( coords ) );

                    if( HEATMAP )
                    {
                        kvs::ValueArray<kvs::UInt8> colors = createHeatmapColors( tables[time_index], cmap );
                        sphere->setColors( colors );
                    }
                }

                screen.redraw();
            }
            else
            {
                playing = false;
            }
        }
    }, delta_time );
    screen.addEvent( &event );

    screen.show();

    return app.run();
}
