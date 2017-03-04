#include "StdAfx.h"
#include "Route.h"

Route::Route(const uint matrix_size_y, const uint matrix_size_x, const char* vals)
{
	simulation_mode = false;
	slow_moves = 0;
	failed_slow_moves = 0;

	thin_x = 0;
	thin_y = 0;
	thin_move = NULL;
	
	filled_count = 0;
	wall_count = 0;

	total_moves = 0;
	current_move = NULL;

	size_y = matrix_size_y;
	size_x = matrix_size_x;

	init_matrix(vals);

	wall_group_index_size = index_wall_groups();

	wall_groups = new uint[wall_group_index_size];
	memset(wall_groups, 0, wall_group_index_size * sizeof(wall_groups[0]));

	start_point_y = 1;
	start_point_x = 1;
	current_x = 1;
	current_y = 1;

	done = false;
}

Route::Route(const uint start_y, const uint start_x, const Route *route, FailCondition*** fail_condition)
{
	simulation_mode = false;
	m_fail_condition = fail_condition;

	slow_moves = 0;
	failed_slow_moves = 0;

	thin_x = 0;
	thin_y = 0;
	thin_move = NULL;
	
	filled_count = 0;
	wall_count = 0;

	total_moves = 0;
	current_move = NULL;

	size_y = route->size_y;
	size_x = route->size_x;

	init_matrix((const uint**)route->matrix);

	wall_group_index_size = route->wall_group_index_size;
	wall_groups = new uint[wall_group_index_size];
	memcpy_s(wall_groups, wall_group_index_size * sizeof(route->wall_groups[0]), route->wall_groups, wall_group_index_size * sizeof(route->wall_groups[0]));

	start_point_y = start_y;
	start_point_x = start_x;

	current_y = start_y;
	current_x = start_x;

	wall_count = route->wall_count;

	done = false;
}

Route::~Route(void)
{
	if (matrix)
	{
		for(uint i = 0; i < size_y; i++)
			delete[] matrix[i];
		delete[] matrix;
	}
	delete[] wall_groups;
}

void Route::init_matrix(const char* vals)
{
	matrix = new uint*[size_y];
	for(uint i = 0; i < size_y; i++)
	{
		matrix[i] = new uint[size_x];
		for(uint j = 0; j < size_x; j++)
		{
			matrix[i][j] = 0;
		}
	}

	for(uint i = 0; i < size_x; i++)
	{
		matrix[0][i] = WALL;
		matrix[1][i]++;
	}

	for(uint i = 1; i < size_y - 1; i++)
	{
		matrix[i][0] = WALL;
		matrix[i][1]++;

		for(uint j = 1; j < size_x - 1; j++)
		{
			if (*vals == 'X')
			{
				wall_count++;
				matrix[i][j] = WALL;
			}

			vals++;
		}
		matrix[i][size_x-1] = WALL;
		if (matrix[i][size_x-2] < EMPTY_CHECK)
			matrix[i][size_x-2]++;
	}

	for(uint i = 0; i < size_x; i++)
	{
		matrix[size_y-1][i] = WALL;
		if (matrix[size_y-2][i] < EMPTY_CHECK)
			matrix[size_y-2][i]++;
	}
}


void Route::init_matrix(const uint** in_matrix)
{
	matrix = new uint*[size_y];

	
	for(uint i = 0; i < size_y; i++)
	{
		matrix[i] = new uint[size_x];
		for(uint j = 0; j < size_x; j++)
		{
			matrix[i][j] = in_matrix[i][j];
		}
	}
}

void Route::print_matrix(const uint y, const uint x)
{
	printf(" ");
	for(uint i = 0; i < size_x; i++)
	{
		printf("%d", i%10);
	}
	printf("\n");


	for(uint i = 0; i < size_y; i++)
	{
		/*printf("%d", i%10);

		for(uint j = 0; j < size_x; j++)
		{
			char ch;

			if (matrix[i][j] < EMPTY_CHECK)
				ch = ' ';
			else if (matrix[i][j] > WALL && matrix[i][j] < WALL + 10)
				ch = 48 + matrix[i][j] - WALL;
			else
				ch = (char)178;

			if (i == current_y && j == current_x)
			{
				printf("%c", '*');
			}
			else if (i == y && j == x)
			{
				printf("%c", 'X');
			}
			else
			{
				printf("%c", ch);
			}
		}

		printf("       ");*/

		printf("%d", i%10);

		for(uint j = 0; j < size_x; j++)
		{
			char ch;

			if (matrix[i][j] == WALL)
				ch = ' ';
			else if (is_wall_touched(i, j))
				ch = '+';
			else if (matrix[i][j] < EMPTY_CHECK)
				ch = (char)178;
			else if (matrix[i][j] == EMPTY_CHECK)
				ch = '-';
			else
				ch = (char)176;

			if (i == current_y && j == current_x)
			{
				printf("%c", ' ');
			}
			else if (i == y && j == x)
			{
				printf("%c", 'X');
			}
			else
			{
				printf("%c", ch);
			}
		}


		printf("\n");
	}

	printf("filled count: %d\n", filled_count);
	printf("total count: %d\n", filled_count + wall_count);
}

uint Route::get_oposite_dir(const uint dir)
{
	switch (dir)
	{
	case UP:
		return DOWN;
	case DOWN:
		return UP;
	case LEFT:
		return RIGHT;
	case RIGHT:
		return LEFT;
	default:
		assert(false);
		return 0;
	}
}

uint* Route::get_cell(const uint direction, const uint count)
{
	switch (direction)
	{
	case UP:
		return &matrix[current_y-count][current_x];
	case DOWN:
		return &matrix[current_y+count][current_x];
	case LEFT:
		return &matrix[current_y][current_x-count];
	case RIGHT:
		return &matrix[current_y][current_x+count];
	default:
		print_matrix();
		assert(false);
		return &matrix[0][0];
	}
}

uint Route::get_cell_shift(const uint y_shift, const uint x_shift)
{
	uint x_val = current_x + x_shift;
	uint y_val = current_y + y_shift;

	return matrix[y_val][x_val];
}


Move* Route::get_next_move()
{
	if (simulation_mode)
	{
		Move* tmp = new Move(*correct_path, NONE, true, current_move, current_y, current_x);
		correct_path++;
		return tmp;
	}

	uint next_direction;
	uint backup_direction;

	if (!current_move)
		return NULL;

	if (current_move->prev_move)
	{
		next_direction = get_oposite_dir(current_move->prev_move->dir);
		backup_direction = current_move->prev_move->dir;
	}
	else
	{
		if (current_move->dir == UP || current_move->dir == DOWN)
		{
			next_direction = RIGHT;
			backup_direction = LEFT;
		}
		else
		{
			next_direction = UP;
			backup_direction = DOWN;
		}
	}

	bool quick = true;


	if (*get_cell(next_direction) < EMPTY_CHECK)
	{
		if (*get_cell(backup_direction) < EMPTY_CHECK)
			quick = false;
		else
			backup_direction = NONE;
	}
	else
	{
		if (*get_cell(backup_direction) < EMPTY_CHECK)
		{
			next_direction = backup_direction;
			backup_direction = NONE;
		}
		else
		{
			return NULL;
		}
	}
	


	//-------------
	//if (current_move->dir == UP || current_move->dir == DOWN)
	//{
	//	next_direction = RIGHT;
	//	backup_direction = LEFT;
	//}
	//else
	//{
	//	next_direction = UP;
	//	backup_direction = DOWN;
	//}

	//uint next_direction_count = 0;
	//uint backup_direction_count = 0;
	//while (*get_cell(next_direction, next_direction_count+1) < EMPTY_CHECK)
	//	next_direction_count++;
	//while (*get_cell(backup_direction, backup_direction_count+1) < EMPTY_CHECK)
	//	backup_direction_count++;

	//bool quick = false;
	//if (current_move->dir == NONE || (!backup_direction_count && !next_direction_count))
	//	return NULL;

	//if (!backup_direction_count)
	//{
	//	backup_direction = NONE;
	//	quick = true;
	//}

	//if (!next_direction_count)
	//{
	//	next_direction = backup_direction;
	//	backup_direction = NONE;
	//	quick = true;
	//}

	//if (!quick && next_direction_count > backup_direction_count)
	//{
	//	uint tmp = next_direction;
	//	next_direction = backup_direction;
	//	backup_direction = tmp;
	//}

	//-------------
	Move* tmp = new Move(next_direction, backup_direction, quick, current_move, current_y, current_x);

	return tmp;
}

void Route::touch_wall(const uint y, const uint x)
{
	if (matrix[y][x] > WALL && matrix[y][x] < MOVED)
	{
		wall_groups[matrix[y][x] - WALL - 1]++;
	}
}

bool Route::is_wall_touched(const uint y, const uint x)
{
	if (matrix[y][x] > WALL && matrix[y][x] < MOVED)
	{
		return (wall_groups[matrix[y][x] - WALL - 1] > 0);
	}
	return false;
}

bool Route::is_wall_untouched(const uint y, const uint x)
{
	if (matrix[y][x] > WALL && matrix[y][x] < MOVED && wall_groups[matrix[y][x] - WALL - 1] == 0)
		return true;
	return false;
}

bool Route::is_wall_touched_or_cell_moved(const uint y, const uint x)
{
	if (matrix[y][x] >= MOVED)
		return true;
	return is_wall_touched(y, x);
}

bool Route::is_wall_touched_or_cell_moved(const uint value)
{
	if (value >= MOVED)
		return true;
	else if ((value > WALL && wall_groups[value - WALL - 1] > 0))
		return true;
	return false;
}

bool Route::index_wall_groups_recursive(const uint y, const uint x, const uint value)
{
	if (y < size_y && x < size_x && matrix[y][x] == WALL)
	{
		matrix[y][x]+=value;

		index_wall_groups_recursive(y-1, x, value);
		index_wall_groups_recursive(y-1, x+1, value);
		index_wall_groups_recursive(y, x+1, value);
		index_wall_groups_recursive(y+1, x+1, value);
		index_wall_groups_recursive(y+1, x, value);
		index_wall_groups_recursive(y+1, x-1, value);
		index_wall_groups_recursive(y, x-1, value);
		index_wall_groups_recursive(y-1, x-1, value);

		return true;
	}
	return false;
}

uint Route::index_wall_groups()
{
	uint wall_group_count = 1;
	for(uint i = 0; i < size_y; i++)
	{
		for(uint j = 0; j < size_x; j++)
		{
			if (index_wall_groups_recursive(i, j, wall_group_count))
				wall_group_count++;
		}
	}
	return --wall_group_count;
}

FailCondition* Route::check_wall_thin(const uint wall_y, const uint wall_x, const uint y, const uint x, const uint check_dir, const uint left)
{
	if (matrix[y][x] >= EMPTY_CHECK)
	{
		return NULL;
	}

	uint side_y;
	uint side_x;
	uint next_turn;

	if (matrix[y][x] < EMPTY_CHECK && cell_wall_count(y, x) > 2)
	{
		if (thin_move)
		{
			if (thin_y != y || thin_x != x)
			{
				//print_matrix();
				m_tmp_condition.unsetCondition();
				m_tmp_condition.setCondition(3, left, true,
					Coordinate(y, x, EMPTY_STATUS),
					Coordinate(y + TURN_FW[check_dir][0], x + TURN_FW[check_dir][1], TOUCHED_WALL_STATUS),
					Coordinate(wall_y, wall_x, WALL_STATUS));
				return &m_tmp_condition;
			}
		}
		else
		{
			//print_matrix();
			set_cell_thin(y, x);
			return NULL;
		}
	}

	//side y and x are used to check wall here
	side_y = y + TURN[left][1][check_dir][0];
	side_x = x + TURN[left][1][check_dir][1];
	if (is_wall_touched_or_cell_moved(side_y, side_x) && !check_thin(y, x, side_y, side_x, left))
	{
		//print_matrix();
		m_tmp_condition.unsetCondition();
		m_tmp_condition.setCondition(3, left, true,
			Coordinate(y, x, EMPTY_STATUS),
			Coordinate(side_y, side_x, get_cell_status(side_y, side_x)),
			Coordinate(wall_y, wall_x, WALL_STATUS));
		return &m_tmp_condition;
	}

	//finding next empty cell
	for (uint i = 0; i < CHECK_COUNT - 2; i++)
	{
		side_y = y + TURN[left][i][check_dir][0];
		side_x = x + TURN[left][i][check_dir][1];

		if (matrix[side_y][side_x] < EMPTY_CHECK)
		{
			next_turn = TURN[left][i][check_dir][2];
			break;
		}
	}

	//special case when wall creates thin by itself
	if (matrix[side_y][side_x] >= EMPTY_CHECK)
	{
		//print_matrix();
		return NULL;
	}

	return check_wall_thin_internal(wall_y, wall_x, side_y, side_x, next_turn, left);
}

FailCondition* Route::check_wall_thin_internal(const uint wall_y, const uint wall_x, const uint y, const uint x, const uint check_dir, const uint left)
{
	uint side_y;
	uint side_x;

	uint next_y;
	uint next_x;
	uint next_turn;

	for (uint i = 0; i < CHECK_COUNT - 1; i++)
	{
		next_y = y + TURN[left][i][check_dir][0];
		next_x = x + TURN[left][i][check_dir][1];
		if ((current_y == next_y && current_x == next_x) || matrix[next_y][next_x] == MOVED)
			return NULL;
		else if (matrix[next_y][next_x] < EMPTY_CHECK)
		{
			next_turn = TURN[left][i][check_dir][2];
			break;
		}
	}

	//wall has thin by itself
	if (matrix[next_y][next_x] >= EMPTY_CHECK)
	{
		if (thin_move)
		{
			//print_matrix(thin_y, thin_x);
			m_tmp_condition.unsetCondition();
			m_tmp_condition.setCondition(2, left, true,
				Coordinate(y, x, EMPTY_STATUS),
				Coordinate(wall_y, wall_x, WALL_STATUS));
			return &m_tmp_condition;
		}
		else
		{
			//print_matrix(thin_y, thin_x);
			set_cell_thin(y, x);
			return NULL;
		}
	}

	for (uint i = CHECK_COUNT - 2; i > 0; i--)
	{
		side_y = y + TURN[left][i][check_dir][0];
		side_x = x + TURN[left][i][check_dir][1];
		if (is_wall_touched_or_cell_moved(side_y, side_x))
		{
			if (check_thin(y, x, side_y, side_x, left))
				return NULL;
			else
			{
				m_tmp_condition.unsetCondition();
				m_tmp_condition.setCondition(3, left, true,
					Coordinate(y, x, EMPTY_STATUS),
					Coordinate(side_y, side_x, get_cell_status(side_y, side_x)),
					Coordinate(wall_y, wall_x, WALL_STATUS));
				return &m_tmp_condition;
			}
		}
	}

	return check_wall_thin_internal(wall_y, wall_x, next_y, next_x, next_turn, left);
}

uint Route::cell_wall_count(const uint y, const uint x)
{
	byte wall_count = 0;

	if (matrix[y+1][x] > EMPTY_CHECK || (y+1 == current_y && x == current_x))
		wall_count++;
	if (matrix[y-1][x] > EMPTY_CHECK || (y-1 == current_y && x == current_x))
		wall_count++;
	if (matrix[y][x+1] > EMPTY_CHECK || (y == current_y && x+1 == current_x))
		wall_count++;
	if (matrix[y][x-1] > EMPTY_CHECK || (y == current_y && x-1 == current_x))
		wall_count++;

	return wall_count;
}

//y x cell is checked to be empty
//true on connected thin
bool Route::check_thin(const uint y, const uint x, const uint wall_y, const uint wall_x, const uint left)
{
	if (!thin_move)
	{
		set_cell_thin(y, x);
	}
	else if ((thin_y != y || thin_x != x))
	{
		uint direction;
		if (wall_y > y)
			direction = DOWN;
		else if (wall_y < y)
			direction = UP;
		else if (wall_x > x)
			direction = RIGHT;
		else
			direction = LEFT;

		return check_connection(y, x, TURN[left][0][direction][2], y, x, TURN[left^1][0][direction][2], left);
	}
	return true;
}

//true on connected
bool Route::check_connection(const uint y1, const uint x1, const uint dir1, const uint y2, const uint x2, const uint dir2, const uint left)
{
	uint right = left ^ 1;
	//print_matrix(y1, x1);
	uint next_y1 = 0, next_x1 = 0, next_dir1 = 0;
	if (dir1)
		for (uint i = 0; i < CHECK_COUNT; i++)
		{
			next_y1 = y1 + TURN[right][i][dir1][0];
			next_x1 = x1 + TURN[right][i][dir1][1];
			if (y1 == current_y && x1 == current_x) /*|| matrix[next_y1][next_x1] == MOVED)*/
				return true;
			else if (y1 == thin_y && x1 == thin_x)
			{
				//print_matrix();
				return false;
			}
			else if (matrix[next_y1][next_x1] < EMPTY_CHECK)
			{
				next_dir1 = TURN[right][i][dir1][2];
				break;
			}
		}
	//else
	//	print_matrix();

	uint next_y2 = 0, next_x2 = 0, next_dir2 = 0;
	if (dir2)
		for (uint i = 0; i < CHECK_COUNT; i++)
		{
			next_y2 = y2 + TURN[left][i][dir2][0];
			next_x2 = x2 + TURN[left][i][dir2][1];
			if (y2 == thin_y && x2 == thin_x)
				return true;
			else if (y2 == current_y && x2 == current_x) /*|| matrix[next_y2][next_x2] == MOVED)*/
			{
				//print_matrix();
				return false;
			}
			else if (matrix[next_y2][next_x2] < EMPTY_CHECK)
			{
				next_dir2 = TURN[left][i][dir2][2];
				break;
			}
		}
	//else
	//	print_matrix(y1, x1);

	//print_matrix(y1, x1);
	//print_matrix(y2, x2);

	return check_connection(next_y1, next_x1, next_dir1, next_y2, next_x2, next_dir2, left);
}

CellStatus Route::get_cell_status(const uint y, const uint x)
{
	if (matrix[y][x] < 4)
		return EMPTY_STATUS;
	else if (matrix[y][x] < MOVED)
		if (is_wall_touched(y, x))
			return TOUCHED_WALL_STATUS;
		else
			return WALL_STATUS;
	else
		return MOVED_STATUS;
}

bool Route::check_cell(const uint next_val)
{
	bool ret_val = true;

	if (!current_move->count && checkCondition())
	{
		//print_matrix(thin_y, thin_x);
		return false;
	}

	uint side_left_y = current_y + TURN_LEFT[current_move->dir][0];
	uint side_left_x = current_x + TURN_LEFT[current_move->dir][1];

	uint side_right_y = current_y + TURN_RIGHT[current_move->dir][0];
	uint side_right_x = current_x + TURN_RIGHT[current_move->dir][1];

	uint fw_y = current_y + TURN_FW[current_move->dir][0];
	uint fw_x = current_x + TURN_FW[current_move->dir][1];

	uint back_y = current_y + TURN_BACK[current_move->dir][0];
	uint back_x = current_x + TURN_BACK[current_move->dir][1];

	uint back_y2 = current_y + TURN_BACK[current_move->dir][0] + TURN_BACK[current_move->dir][0];
	uint back_x2 = current_x + TURN_BACK[current_move->dir][1] + TURN_BACK[current_move->dir][1];

	uint side_left_y2 = current_y + TURN_LEFT[current_move->dir][0] + TURN_LEFT[current_move->dir][0];
	uint side_left_x2 = current_x + TURN_LEFT[current_move->dir][1] + TURN_LEFT[current_move->dir][1];

	uint side_right_y2 = current_y + TURN_RIGHT[current_move->dir][0] + TURN_RIGHT[current_move->dir][0];
	uint side_right_x2 = current_x + TURN_RIGHT[current_move->dir][1] + TURN_RIGHT[current_move->dir][1];

	if (thin_move && current_x == thin_x && current_y == thin_y)
	{
		//print_matrix();
		assert(thin_move->thin);
		thin_move->thin = false;
		thin_move = NULL;
		thin_x = 0;
		thin_y = 0;
	}

	if (matrix[side_left_y][side_left_x] < EMPTY_CHECK && is_wall_touched_or_cell_moved(side_left_y2, side_left_x2))
	{
		if (next_val < WALL)
		{
			if (!check_thin(side_left_y, side_left_x, side_left_y2, side_left_x2, 1))
			{
				//print_matrix(thin_y, thin_x);
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(3, 1, true,
						Coordinate(side_left_y, side_left_x, get_cell_status(side_left_y, side_left_x)),
						Coordinate(side_left_y2, side_left_x2, get_cell_status(side_left_y2, side_left_x2)),
						Coordinate(fw_y, fw_x, EMPTY_STATUS));
				return false;
			}
		}
	}

	if (matrix[side_right_y][side_right_x] < EMPTY_CHECK && is_wall_touched_or_cell_moved(side_right_y2, side_right_x2))
	{
		if (next_val < WALL)
		{
			if (!check_thin(side_right_y, side_right_x, side_right_y2, side_right_x2, 0))
			{
				//if (m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].m_coordinates)
				//	print_matrix(thin_y, thin_x);
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(3, 0, true,
						Coordinate(side_right_y, side_right_x, EMPTY_STATUS),
						Coordinate(side_right_y2, side_right_x2, get_cell_status(side_right_y2, side_right_x2)),
						Coordinate(fw_y, fw_x, EMPTY_STATUS)
						
						);	

	//print_matrix(thin_y, thin_x);
				return false;
			}
		}
	}

	if ((current_move->count > 1 && is_wall_touched_or_cell_moved(side_left_y, side_left_x)) &&
			(get_cell_shift(TURN_LEFT[current_move->dir][0] + TURN_BACK[current_move->dir][0], TURN_LEFT[current_move->dir][1] + TURN_BACK[current_move->dir][1]) < EMPTY_CHECK))
			//empty cells left that could not be reached
	{
		//if (m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].m_coordinates)
		//	print_matrix(thin_y, thin_x);
		uint tmp_y = current_y + TURN_LEFT[current_move->dir][0] + TURN_BACK[current_move->dir][0];
		uint tmp_x = current_x + TURN_LEFT[current_move->dir][1] + TURN_BACK[current_move->dir][1];
		m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(3, 0, false,
				Coordinate(side_left_y, side_left_x, get_cell_status(side_left_y, side_left_x)),
				Coordinate(tmp_y, tmp_x, EMPTY_STATUS),
				Coordinate(current_y, current_x, EMPTY_STATUS));

		return false;
	}
		
	if ((current_move->count > 1 && is_wall_touched_or_cell_moved(side_right_y, side_right_x)) &&
			(get_cell_shift(TURN_RIGHT[current_move->dir][0] + TURN_BACK[current_move->dir][0], TURN_RIGHT[current_move->dir][1] + TURN_BACK[current_move->dir][1]) < EMPTY_CHECK))
			//empty cells left that could not be reached
	{
		//if (m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].m_coordinates)
		//	print_matrix(thin_y, thin_x);
		uint tmp_y = current_y + TURN_RIGHT[current_move->dir][0] + TURN_BACK[current_move->dir][0];
		uint tmp_x = current_x + TURN_RIGHT[current_move->dir][1] + TURN_BACK[current_move->dir][1];
		m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(3, 0, false,
				Coordinate(side_right_y, side_right_x, get_cell_status(side_right_y, side_right_x)),
				Coordinate(tmp_y, tmp_x, EMPTY_STATUS),
				Coordinate(current_y, current_x, EMPTY_STATUS));

		return false;
	}


	if (is_wall_touched_or_cell_moved(next_val) && (matrix[side_left_y][side_left_x] < EMPTY_CHECK) &&
			(matrix[side_right_y][side_right_x] < EMPTY_CHECK))
			//wall ahead is touched and there are empty cells from both sides
	{
		//if (m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].m_coordinates)
		//	print_matrix(thin_y, thin_x);
		m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(3, 0, false,
				Coordinate(fw_y, fw_x, get_cell_status(fw_y, fw_x)),
				Coordinate(side_left_y, side_left_x, get_cell_status(side_left_y, side_left_x)),
				Coordinate(side_right_y, side_right_x, get_cell_status(side_right_y, side_right_x)));

		return false;
	}

	uint three_wall_y = 0;
	uint three_wall_x = 0;

	if (is_wall_untouched(side_left_y, side_left_x))
	{
		//print_matrix();
		if (current_move->count)
		{
			if (check_wall_thin(side_left_y, side_left_x, side_left_y + TURN_BACK[current_move->dir][0], side_left_x + TURN_BACK[current_move->dir][1],
						get_oposite_dir(current_move->dir), 1))
			{
//if (current_y == 34 && current_x == 14 && current_move->dir == 4)
//	print_matrix(thin_y, thin_x);
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(m_tmp_condition);
				return false;
			}
		}
		else
		{
			//print_matrix();
			if (check_wall_thin(side_left_y, side_left_x, current_y + TURN_BACK[current_move->dir][0], current_x + TURN_BACK[current_move->dir][1],
						get_oposite_dir(current_move->dir), 1))
			{
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(m_tmp_condition);
				return false;
			}

		}
	}

	if (is_wall_untouched(side_right_y, side_right_x))
	{
		if (current_move->count)
		{
			if (check_wall_thin(side_right_y, side_right_x, side_right_y + TURN_BACK[current_move->dir][0], side_right_x + TURN_BACK[current_move->dir][1],
					get_oposite_dir(current_move->dir), 0))
			{
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(m_tmp_condition);
				//print_matrix();
				return false;
			}
		}
		else
		{
			//print_matrix();
			if (check_wall_thin(side_right_y, side_right_x, current_y + TURN_BACK[current_move->dir][0], current_x + TURN_BACK[current_move->dir][1],
					get_oposite_dir(current_move->dir), 0))
			{
				m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1].setCondition(m_tmp_condition);
				return false;
			}
		}
	}

	if (next_val < WALL)
	{
		touch_wall(side_left_y, side_left_x);
		touch_wall(side_right_y, side_right_x);
	}
	return ret_val;
}

bool Route::set_moved()
{
	while (matrix[current_y][current_x] < WALL)
	{
		uint last_val = *get_cell(current_move->dir);
		if (!check_cell(last_val))
			return false;

		if (last_val >= WALL)
			return true;

		matrix[current_y][current_x] += MOVED;
		current_move->count++;
		filled_count++;

		switch (current_move->dir)
		{
		case UP:
			current_y--;
			break;
		case DOWN:
			current_y++;
			break;
		case LEFT:
			current_x--;
			break;
		case RIGHT:
			current_x++;
			break;
		}
	}
	return true;
}

void Route::untouch_wall(const uint y, const uint x)
{
	if (matrix[y][x] > WALL && matrix[y][x] < MOVED)
	{
		wall_groups[matrix[y][x] - WALL - 1]--;
	}
}

bool Route::get_backup()
{	
	//print_matrix(current_y, current_x);
	if (current_move->backup_dir)
	{
		//print_matrix();
		failed_slow_moves++;

		//if (current_move->prev_move)
		//	if (current_move->prev_move->check_next_move_backup)
		//	{
		//		current_move->prev_move->check_cell_y = current_move->prev_move->check_backup_cell_y;
		//		current_move->prev_move->check_cell_x = current_move->prev_move->check_backup_cell_x;
		//	}
		//	else
		//	{
		//		current_move->prev_move->check_next_move = false;
		//	}

		current_move->dir = current_move->backup_dir;
		current_move->backup_dir = NONE;
		return true;
	}
	else
	{
		if (!current_move->quick)
		{
			assert(slow_moves > 0);
			//print_matrix();
			slow_moves--;
			failed_slow_moves--;
		}

		Move* tmp = current_move;
		current_move = current_move->prev_move;
		delete tmp;
		if (current_move)
			return step_back();
		else
			return false;
	}
}

bool Route::step_back()
{
	uint x1 = current_x;
	uint y1 = current_y;
	uint x2 = current_x;
	uint y2 = current_y;

	//255 is used as -1
	uint x_inc = 0;
	uint y_inc = 0;

	//print_matrix();

	switch (current_move->dir)
	{
	case UP:
		//left and right, for wall checking
		x1++;
		x2--;
		y_inc++;
		break;
	case DOWN:
		//left and right, for wall checking
		x1++;
		x2--;
		y_inc--;
		break;
	case LEFT:
		y1--;
		y2++;
		x_inc++;
		break;
	case RIGHT:
		y1--;
		y2++;
		x_inc--;
		break;
	}

	for (uint n = 0; n < current_move->count; n++)
	{
		//print_matrix();
		//stepping back, as current is not set
		current_y+=y_inc;
		current_x+=x_inc;
		
		y1+=y_inc;
		y2+=y_inc;
		x1+=x_inc;
		x2+=x_inc;

		untouch_wall(y1, x1);
		untouch_wall(y2, x2);
		
		matrix[current_y][current_x] -= MOVED;
	}
	filled_count -= current_move->count;
	current_move->count = 0;

	if (current_move->thin)
	{
		if (thin_move)
		{
			thin_move = NULL;
			thin_x = 0;
			thin_y = 0;
		}
		current_move->thin = false;
	}
	return get_backup();
}

bool Route::start_solve(const uint direction)
{	
	if (matrix[start_point_y][start_point_x] >= WALL)
	{
		return false;
	}

	current_y = start_point_y;
	current_x = start_point_x;

	if (*get_cell(direction) >= EMPTY_CHECK)
	{
		return false;
	}

	current_move = new Move(direction, NONE, false, NULL, current_y, current_x);
	slow_moves++; //since first move is not quick

	//print_matrix();

	uint y = current_y;
	uint x = current_x;

	touch_wall(y-1, x);
	touch_wall(y-1, x+1);
	touch_wall(y, x+1);
	touch_wall(y+1, x+1);
	touch_wall(y+1, x);
	touch_wall(y+1, x-1);
	touch_wall(y, x-1);
	touch_wall(y-1, x-1);

	uint side_left_y = current_y + TURN_LEFT[current_move->dir][0];
	uint side_left_x = current_x + TURN_LEFT[current_move->dir][1];

	uint side_right_y = current_y + TURN_RIGHT[current_move->dir][0];
	uint side_right_x = current_x + TURN_RIGHT[current_move->dir][1];

	uint back_y = current_y + TURN_BACK[current_move->dir][0];
	uint back_x = current_x + TURN_BACK[current_move->dir][1];

	if (matrix[side_left_y][side_left_x] < EMPTY_CHECK && cell_wall_count(side_left_y, side_left_x) == 3)
		set_cell_thin(side_left_y, side_left_x);
	if (matrix[side_right_y][side_right_x] < EMPTY_CHECK && cell_wall_count(side_right_y, side_right_x) == 3)
		if (!thin_move)
			set_cell_thin(side_right_y, side_right_x);
		else
			return false;
	if (matrix[back_y][back_x] < EMPTY_CHECK && cell_wall_count(back_y, back_x) == 3)
		if (!thin_move)
			set_cell_thin(back_y, back_x);
		else
			return false;

	filled_count = 0;
	return true;
}

bool Route::solve()
{
	uint max_filled_count = 0;
	while(1)
	{
		Move* tmp;
		total_moves++;

		//if (simulation_mode)
		//	print_matrix();

//		if (current_x == 72 && current_y == 51)
	//		print_matrix();


		//if (total_moves > 9873)
		//{
		//	print_matrix(thin_y, thin_x);
		//	int i = 0;
		//	i++;
		//}

		//if (total_moves%100000 == 99999)
		//{
		//	print_matrix(thin_y, thin_x);
		//	int i = 0;
		//	i++;
		//}

		if (set_moved())
		{
			if (filled_count > max_filled_count)
				max_filled_count = filled_count;
			tmp = get_next_move();
			if (tmp)
			{
				if (!tmp->quick)
					slow_moves++;

				current_move = tmp;
			}
			else
			{
				if (filled_count + wall_count + 2 > (size_x-2) * (size_y-2))
				{
					//print_matrix();
					printf("done total_moves %d, failed_moves %d /moves %d = %f\n\n", total_moves, failed_slow_moves, slow_moves, (double)failed_slow_moves/slow_moves);
					done = true;
					return true;
				}
				else
				{
					if (!step_back())
					{
						return false;
					}
				}
			}
		}
		else
		{
				//if (filled_count + wall_count + 20 > (size_x-2) * (size_y-2))
				//{
				//	//print_matrix();
				//	printf("done total_moves %d, failed_moves %d /moves %d = %f\n\n", total_moves, failed_slow_moves, slow_moves, (double)failed_slow_moves/slow_moves);
				//	done = true;
				//	return true;
				//}
			if (!step_back())
			{
				return false;
			}
		}
	}
	return false;
}

char* Route::get_result(const int max_size)
{
	Move *move = current_move;

	char* tmp_string = new char[max_size];
	char* output_pos = tmp_string + max_size - 1;

	*output_pos = NULL;

	int step_count = 0;

	while (move)
	{
		if (++step_count >= max_size)
		{
			delete[] tmp_string;
			return NULL;
		}

		char direction;
		switch (move->dir)
		{
		case 1:
			direction = 'U';
			break;
		case 2:
			direction = 'R';
			break;
		case 3:
			direction = 'D';
			break;
		case 4:
			direction = 'L';
			break;
		default:
			assert(false);
		}

		//printf("%c %d |\n", direction, move->count);

		if (!move->quick && !simulation_mode)
		{
			output_pos--;
			*output_pos = direction;
		}
		
		Move *tmp = move;
		move = move->prev_move;
		delete tmp;
	}

	size_t size = strlen(output_pos) + 1;
	char* result = new char[size];
	strncpy_s(result, size, output_pos, size);
	delete[] tmp_string;

	//printf("step count: %d, start_y:%i, start_x:%i\n", step_count, start_point_y, start_point_x);

	return result;
}

void Route::get_start_coordinates(uint &y, uint &x)
{
	y = start_point_y - 1;
	x = start_point_x - 1;
}

void Route::set_cell_thin(const uint y, const uint x)
{
	thin_x = x;
	thin_y = y;
	current_move->thin = true;
	thin_move = current_move;
}

bool Route::checkCondition()
{
	FailCondition& condition = m_fail_condition[current_move->start_y][current_move->start_x][current_move->dir-1];

	if (!condition.m_coordinates)
		return false;
	
	if (condition.m_thin && !thin_move)
	{
		return false;
	}

	if (condition.m_thin)
	{
		uint tmp_y = current_y;
		uint tmp_x = current_x;
		do
		{
			tmp_y += TURN_FW[current_move->dir][0];
			tmp_x += TURN_FW[current_move->dir][1];
			if (tmp_y == thin_y && tmp_x == thin_x)
			{
				return false;
			}
		}
		while (matrix[tmp_y][tmp_x] < WALL);
	}

	for (uint i = 0; i < condition.m_count; i++)
	{
		if (get_cell_status(condition.m_coordinates[i].m_y, condition.m_coordinates[i].m_x) != 
				condition.m_coordinates[i].m_status)
			return false;
	}

	if (condition.m_thin && condition.m_count == 3)
		return !check_thin(condition.m_coordinates[0].m_y, condition.m_coordinates[0].m_x,
			condition.m_coordinates[1].m_y, condition.m_coordinates[1].m_x, condition.m_left);

	return true;
}

