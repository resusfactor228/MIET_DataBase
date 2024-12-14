#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <postgresql/libpq-fe.h>

#define BUFFER_SIZE 512

//Функция выхода из программы с сообщением об ошибке.
void err_exit(PGconn *conn) {
	PQfinish(conn); //Закрытие соединения с сервером базы данных
	exit(1); //Функция возвращает значение 1 -> программа завершилась аварийно
}

void print_res(PGresult* res) {
	int rows = PQntuples(res);
    int cols = PQnfields(res);
	printf("=================\n");
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            printf("%s ", PQgetvalue(res, i, j));
        }
        printf("\n");
    }
	printf("=================\n");
}

/*
 Функция печати результата запроса на экран
 *conn -> дескриптор подключения к серверу
 query -> текст запроса
 */

void print_query(PGconn *conn, char* query) {

	PGresult *res = PQexec(conn, query); //Передача запроса на сервер
	int rows = PQntuples(res); //Получение числа строк в результате запроса
	int cols = PQnfields(res); //Получение числа столбцов в результате запроса
	
	// Проверка наличия возвращенных запросом строк.
	// В случае их отсутствие - аварийное завершение программы
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		printf("No data retrieved\n");
		PQclear(res);
		err_exit(conn);
	}
	
	//Вывод результата запроса на экран
	for(int i=0; i<rows; i++) {
		for(int j=0; j<cols; j++) {
			printf("%s ", PQgetvalue(res, i, j));
		}
		printf("\n");
	}
	PQclear(res);
}

void print_group(PGconn *conn, const char* f_name) {
    char query[BUFFER_SIZE] = {0};
	const char* paramValues[1] = {f_name};
    
    strcpy(query, "SELECT * FROM students WHERE students_group_number = $1;");

    PGresult *res = PQexecParams(conn, query, 1, NULL, paramValues, NULL, NULL, 0); 

 	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("No data retrieved\n");        
        PQclear(res);
        err_exit(conn);
    }

    print_res(res);
    PQclear(res);
}

void update_mark(PGconn* conn, const char* surname, const char* name, const char* groups_number, const char* field, const char* mark) {
	char query[BUFFER_SIZE * 2] = {0};
	const char* paramValues[] = {mark, surname, name, groups_number, field};
	
	//printf("Entered data: %s %s %s %s %s\n\n", surname, name, groups_number, field, mark);

    strcpy(query, "UPDATE field_comprehensions f_c SET mark = $1 \
					WHERE f_c.student_id IN ( \
						SELECT s.student_id	\
						FROM students s	\
						JOIN field_comprehensions f_c ON s.student_id = f_c.student_id \
						JOIN fields f ON f.field_id = f_c.field \
						WHERE s.last_name = $2 AND s.first_name = $3 AND s.students_group_number = $4 AND f.field_name = $5 LIMIT 1 \
					) AND f_c.field IN ( \
						SELECT f.field_id \
						FROM fields f \
						JOIN field_comprehensions f_c ON f.field_id = f_c.field \
						WHERE f.field_name = $5 \
					);"
	);

    PGresult *res = PQexecParams(conn, query, 5, NULL, paramValues, NULL, NULL, 0); 

 	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("FAILED: %s\n", PQerrorMessage(conn));        
        PQclear(res);
        err_exit(conn);
    }

	printf("\nUPDATE %s\n", PQcmdTuples(res));
    PQclear(res);
}

void insert_mark(PGconn* conn, const char* surname, const char* name, const char* groups_number, const char* field, const char* mark) {
	char query[BUFFER_SIZE] = {0};
	const char* paramValues[] = {mark, surname, name, groups_number, field};

    strcpy(query, "INSERT INTO field_comprehensions (student_id, field, mark) \
					VALUES ( \
						(SELECT student_id FROM students s WHERE s.last_name = $2 AND s.first_name = $3 AND s.students_group_number = $4 LIMIT 1), \
						(SELECT field_id FROM fields f JOIN field_comprehensions f_c ON f.field_id = f_c.field WHERE f.field_name = $5 LIMIT 1), \
						$1 \
					)"
	);

    PGresult *res = PQexecParams(conn, query, 5, NULL, paramValues, NULL, NULL, 0); 

 	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("%s", PQerrorMessage(conn));        
        PQclear(res);
        err_exit(conn);
    }

	printf("\nINSERT %s\n", PQcmdTuples(res));
    PQclear(res);
}

void delete_mark(PGconn* conn, const char* surname, const char* name, const char* groups_number, const char* field) {
	char query[BUFFER_SIZE * 2] = {0};
	const char* paramValues[] = {surname, name, groups_number, field};

    strcpy(query, "DELETE FROM field_comprehensions \
				WHERE field = ( \
					SELECT f.field_id  FROM field_comprehensions f_c \
					JOIN students s ON s.student_id = f_c.student_id \
					JOIN fields f ON f.field_id = f_c.field \
					WHERE s.last_name = $1 AND s.first_name = $2 AND s.students_group_number = $3 AND f.field_name = $4 \
					LIMIT 1 \
				) \
				AND student_id = ( \
					SELECT s.student_id FROM field_comprehensions f_c \
					JOIN students s ON s.student_id = f_c.student_id \
					JOIN fields f ON f.field_id = f_c.field \
					WHERE s.last_name = $1 AND s.first_name = $2 AND s.students_group_number = $3 AND f.field_name = $4 \
					LIMIT 1 \
				)"
	);

    PGresult *res = PQexecParams(conn, query, 4, NULL, paramValues, NULL, NULL, 0); 

 	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("%s", PQerrorMessage(conn));      
        PQclear(res);
        err_exit(conn);
    }

	printf("\nDELETE %s\n", PQcmdTuples(res));
    PQclear(res);
}

void select_record(PGconn* conn, const char* surname, const char* name, const char* groups_number, const char* field) {
	char query[BUFFER_SIZE * 2] = {0};
	PGresult *res = NULL;
	//printf("%s %s %s %s\n", surname, name, groups_number, field);

	if (field) {
		const char* paramValues[] = {surname, name, groups_number, field};
    	strcpy(query, "SELECT s.first_name, s.last_name, s.students_group_number, f.field_name, f_c.mark \
				FROM students s \
				JOIN field_comprehensions f_c ON s.student_id = f_c.student_id \
				JOIN fields f ON f_c.field = f.field_id \
				WHERE s.last_name = $1 AND s.first_name = $2 AND s.students_group_number = $3 AND f.field_name = $4;"
		);
		res = PQexecParams(conn, query, 4, NULL, paramValues, NULL, NULL, 0); 
	} else {
		const char* paramValues[] = {surname, name, groups_number};
    	strcpy(query, "SELECT s.first_name, s.last_name, s.students_group_number, f.field_name, f_c.mark \
				FROM students s \
				JOIN field_comprehensions f_c ON s.student_id = f_c.student_id \
				JOIN fields f ON f_c.field = f.field_id \
				WHERE s.last_name = $1 AND s.first_name = $2 AND s.students_group_number = $3;"
		);
		res = PQexecParams(conn, query, 3, NULL, paramValues, NULL, NULL, 0);
	}

 	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        printf("%s", PQerrorMessage(conn));      
        PQclear(res);
        err_exit(conn);
    }

	print_res(res);
    PQclear(res);
}

int main() {
	printf("=== Welcome to the miniORIOKS ===");

	start:
	int choice = 0;
	int choice2 = 0;
	PGconn *conn = NULL;
	char password[BUFFER_SIZE / 2] = {0};
	char conn_info[BUFFER_SIZE] = {0};

	printf("\n\nPlease, choose desired access rights:\n\n1 - USER\n2 - ADMIN\n3 - Exit\n\n");
	scanf("%d", &choice);
	
	printf("Password: ");
	scanf("%s", password);
	
	switch (choice) {
		case 1:
			sprintf(conn_info, "user='AAD admin' password=%s dbname=students_", password);
			break;
		case 2:
			sprintf(conn_info, "user='AAD admin' password=%s dbname=students_", password);
			break;
		case 3:
			return 0;
	}

	conn = PQconnectdb(conn_info);
	//Проверка статуса подключения. В случае ошибки - аварийное завершение программы
	if (PQstatus(conn) == CONNECTION_BAD) {
		fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn)); 
		err_exit(conn);
	}

	operations:
	if (choice == 1) {
		printf("\n\nUSER'S operations:\n\n1 - PRINT student's marks\n2 - FIND student's record\n3 - Change user\n\n");
		scanf("%d", &choice2);
		if (choice2 == 1 || choice2 == 2) {
			char surname[BUFFER_SIZE] = {0};
			char name[BUFFER_SIZE] = {0};
			char groups_number[BUFFER_SIZE] = {0};
			printf("Surname: ");
			scanf("%s", surname);
			printf("Name: ");
			scanf("%s", name);
			printf("Group's number: ");
			scanf("%s", groups_number);
			if (choice2 == 1) {
				select_record(conn, surname, name, groups_number, NULL);
			} else {
				char field[BUFFER_SIZE] = {0};
				getchar();
				printf("Field: ");
				scanf("%[^\n]", field);
				select_record(conn, surname, name, groups_number, field);
			}
		} else if (choice2 == 3) {
			PQfinish(conn);
			goto start;
		}
		goto operations;
	} else if (choice == 2){ 
		printf("\n\nADMIN'S operations:\n\n1 - INSERT mark\n2 - UPDATE mark\n3 - DELETE mark\n4 - Change user\n\n");
		scanf("%d", &choice2);
		if (choice2 == 1 || choice2 == 2 || choice2 == 3) {
			char surname[BUFFER_SIZE] = {0};
			char name[BUFFER_SIZE] = {0};
			char groups_number[BUFFER_SIZE] = {0};
			char field[BUFFER_SIZE] = {0};
			printf("Surname: ");
			scanf("%s", surname);
			printf("Name: ");
			scanf("%s", name);
			printf("Group's number: ");
			scanf("%s", groups_number);
			getchar();
			printf("Field: ");
			scanf("%[^\n]", field);
			if (choice2 == 1 || choice2 == 2) {
				char mark[BUFFER_SIZE] = {0};
				printf("Mark: ");
				scanf("%s", mark);
				if (choice2 == 1) {
					insert_mark(conn, surname, name, groups_number, field, mark);
				} else if (choice2 == 2) {
					update_mark(conn, surname, name, groups_number, field, mark);
				}
			} else {
				delete_mark(conn, surname, name, groups_number, field);
			}
		} else if (choice2 == 4) {
			PQfinish(conn);
			goto start;
		}
		goto operations;
	}

	// print_query(conn, "SELECT * FROM students LIMIT 5;");
	// char group_name[BUFFER_SIZE] = {0};
	// printf("Enter group's name: ");
	// scanf("%s", group_name);
	// if (strchr(group_name, ';')) {
	// 	printf("Anti injection system reports: symbol \";\" has been used\nPlease, don't use it again.\n");
	// }
	// print_group(conn, group_name);
	//PQfinish(conn);

	return 0;
}