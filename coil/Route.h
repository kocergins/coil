#pragma once

#include "uint.h"

#define CELL_EMPTY 0
#define EMPTY_CHECK 5
#define WALL_CHECK 9
#define WALL 10
#define MOVED 4294967290

enum Direction
{
	NONE,
	UP,
	RIGHT,
	DOWN,
	LEFT
};

enum CellStatus
{
	EMPTY_STATUS,
	WALL_STATUS,
	TOUCHED_WALL_STATUS,
	MOVED_STATUS
};

#define LEFT_CELL_SHIFT {{0, 0, NONE}, {0, -1, LEFT}, {-1, 0, UP}, {0, 1, RIGHT}, {1, 0, DOWN}}
#define FW_CELL_SHIFT {{0, 0, NONE}, {-1, 0, UP}, {0, 1, RIGHT}, {1, 0, DOWN}, {0, -1, LEFT}}
#define RIGHT_CELL_SHIFT {{0, 0, NONE}, {0, 1, RIGHT}, {1, 0, DOWN}, {0, -1, LEFT}, {-1, 0, UP}}

#define BACK_CELL_SHIFT {{0, 0, NONE}, {1, 0, DOWN}, {0, -1, LEFT}, {-1, 0, UP}, {0, 1, RIGHT}}
//TODO: add back shift, and use it in check_cell

const uint TURN_LEFT[5][3] = LEFT_CELL_SHIFT;
const uint TURN_RIGHT[5][3] = RIGHT_CELL_SHIFT;
const uint TURN_FW[5][3] = FW_CELL_SHIFT;
const uint TURN_BACK[5][3] = BACK_CELL_SHIFT;

#define CHECK_COUNT 4

//[left/right][next direction shifts][current direction][y, x, next direction]
const uint TURN[2][CHECK_COUNT][5][3] =	{{LEFT_CELL_SHIFT, FW_CELL_SHIFT, RIGHT_CELL_SHIFT, BACK_CELL_SHIFT},
								{RIGHT_CELL_SHIFT, FW_CELL_SHIFT, LEFT_CELL_SHIFT, BACK_CELL_SHIFT}};

struct Move 
{
	uint dir;
	uint backup_dir;
	uint count;
	bool quick;
	bool thin;
	Move* prev_move;
	uint start_y;
	uint start_x;

	Move(uint direction, uint backup_direction, bool quick_path, Move* previos_move, uint y, uint x) : dir(direction), backup_dir(backup_direction), count(0),
		quick(quick_path), start_y(y), start_x(x), thin(false), prev_move(previos_move)
	{
	};
	
};


struct Coordinate
{
	Coordinate() {};
	Coordinate(byte y, byte x, CellStatus status) : m_y(y), m_x(x), m_status(status)
	{
		
	};

	byte m_y;
	byte m_x;
	CellStatus m_status;
};

class Route;

struct FailCondition
{
	FailCondition()
	{
		m_coordinates = NULL;
		m_count = 0;
		m_thin = false;
	};

	void unsetCondition()
	{
		if (m_coordinates)
		{
			delete[] m_coordinates;
			m_coordinates = NULL;
		}
	}

	void setCondition(uint coordinate_count, uint left, bool thin, ...)
	{
		if (!m_coordinates)
		{
			m_thin = thin;
			m_left = left;
			m_count = coordinate_count;
			m_coordinates = new Coordinate[coordinate_count];
			va_list coordinates;
			va_start(coordinates, thin);
			for (uint i = 0; i < coordinate_count; i++)
			{
				m_coordinates[i] = va_arg(coordinates, const Coordinate);
			}				
			va_end(coordinates);
		}
	};

	void setCondition(FailCondition &condition)
	{
		if (!m_coordinates)
		{
			m_thin = condition.m_thin;
			m_count = condition.m_count;
			m_left = condition.m_left;
			m_coordinates = condition.m_coordinates;
			assert(m_coordinates->m_status < MOVED_STATUS + 1);
			condition.m_coordinates = NULL;
		}		
	};

	~FailCondition()
	{
		unsetCondition();
	}
	Coordinate* m_coordinates;
	uint m_count;
	uint m_left;
	bool m_thin;
};



class Route
{
public:
	Route(const uint matrix_size_y, const uint matrix_size_x, const char* vals);
	Route(const uint start_y, const uint start_x, const Route *route, FailCondition*** fail_condition);
	~Route(void);
	void print_matrix(const uint y=0, const uint x=0);
	bool start_solve(const uint direction);
	bool solve();
	char* get_result(const int max_size);
	void get_start_coordinates(uint &y, uint &x);
	bool done;

	bool simulation_mode;
	char* correct_path;


private:
	void init_matrix(const char* vals);
	void init_matrix(const uint** in_matrix);
	uint get_oposite_dir(const uint dir);
	uint* get_cell(const uint direction, const uint count = 1);
	uint get_cell_shift(const uint y_shift, const uint x_shift);
	Move* get_next_move();
	void touch_wall(const uint y, const uint x);
	bool is_wall_touched(const uint y, const uint x);
	bool is_wall_untouched(const uint y, const uint x);
	bool is_wall_touched_or_cell_moved(const uint y, const uint x);
	bool is_wall_touched_or_cell_moved(const uint value);
	bool index_wall_groups_recursive(const uint y, const uint x, const uint value);
	uint index_wall_groups();
	FailCondition* check_wall_thin(const uint wall_y, const uint wall_x, const uint y, const uint x, const uint check_dir, const uint left);
	FailCondition* check_wall_thin_internal(const uint wall_y, const uint wall_x, const uint y, const uint x, const uint check_dir, const uint left);
	uint cell_wall_count(const uint y, const uint x);
	bool check_thin(const uint y, const uint x, const uint wall_y, const uint wall_x, const uint left);
	bool check_connection(uint y1, uint x1, uint dir1, uint y2, uint x2, uint dir2, const uint left);
	bool check_cell(const uint next_val);
	bool set_moved();
	void untouch_wall(const uint y, const uint x);
	bool step_back();
	bool get_backup();
	uint get_cell_value(const uint y, const uint x);
	void set_cell_thin(const uint y, const uint x);
	CellStatus get_cell_status(const uint y, const uint x);
	bool checkCondition();

	uint **matrix;

	uint *wall_groups;
	uint wall_group_index_size;

	uint size_x, size_y;
	uint start_point_x;
	uint start_point_y;
	uint current_x;
	uint current_y;
	uint filled_count;
	uint wall_count;

	uint total_moves;

	uint thin_x;
	uint thin_y;
	Move* thin_move;

	Move* current_move;

	uint slow_moves;
	uint failed_slow_moves;

	FailCondition m_tmp_condition;


	FailCondition*** m_fail_condition;
};

