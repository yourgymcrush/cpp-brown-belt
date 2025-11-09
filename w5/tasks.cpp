#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <algorithm>

using namespace std;

// // Перечислимый тип для статуса задачи
// enum class TaskStatus {
//   NEW,          // новая
//   IN_PROGRESS,  // в разработке
//   TESTING,      // на тестировании
//   DONE          // завершена
// };

// // Объявляем тип-синоним для map<TaskStatus, int>,
// // позволяющего хранить количество задач каждого статуса
// using TasksInfo = map<TaskStatus, int>;

class TeamTasks {
public:
  // Получить статистику по статусам задач конкретного разработчика
  const TasksInfo& GetPersonTasksInfo(const string& person) const
  {
    return m_personToTasksInfo.at(person);
  }
  
  // Добавить новую задачу (в статусе NEW) для конкретного разработчитка
  void AddNewTask(const string& person)
  {
    m_personToTasksInfo[person][TaskStatus::NEW]++;
  }
  
  // Обновить статусы по данному количеству задач конкретного разработчика,
  // подробности см. ниже
  tuple<TasksInfo, TasksInfo> PerformPersonTasks(
      const string& person, int task_count)
  {
    TasksInfo updatedtasks;
    TasksInfo restTasks;
    TasksInfo finalTasks;
    auto& personTasksInfo = m_personToTasksInfo[person];

    finalTasks = personTasksInfo;
    for(const auto&[type, count] : personTasksInfo)
    {
      if(type == TaskStatus::DONE)
        continue;
      for(int i = 0; i < count; i++)
      {
        if(task_count)
        {
          updatedtasks[static_cast<TaskStatus>(static_cast<int>(type) + 1)]++;
          finalTasks[static_cast<TaskStatus>(static_cast<int>(type) + 1)]++;
          finalTasks[type]--;
          if(!finalTasks[type])
            finalTasks.erase(type);
          task_count--;
        }
        else
        {
          restTasks[type]++;
        }
      }      
    }
    personTasksInfo = finalTasks;

    return {updatedtasks, restTasks};
  }
private:
  std::unordered_map<std::string, TasksInfo> m_personToTasksInfo;
};


// // Принимаем словарь по значению, чтобы иметь возможность
// // обращаться к отсутствующим ключам с помощью [] и получать 0,
// // не меняя при этом исходный словарь
// void PrintTasksInfo(TasksInfo tasks_info) {
//   cout << tasks_info[TaskStatus::NEW] << " new tasks" <<
//       ", " << tasks_info[TaskStatus::IN_PROGRESS] << " tasks in progress" <<
//       ", " << tasks_info[TaskStatus::TESTING] << " tasks are being tested" <<
//       ", " << tasks_info[TaskStatus::DONE] << " tasks are done" << endl;
// }

// int main() {
//   TeamTasks tasks;
//   tasks.AddNewTask("Ilia");
//   for (int i = 0; i < 3; ++i) {
//     tasks.AddNewTask("Ivan");
//   }
//   cout << "Ilia's tasks: ";
//   PrintTasksInfo(tasks.GetPersonTasksInfo("Ilia"));
//   cout << "Ivan's tasks: ";
//   PrintTasksInfo(tasks.GetPersonTasksInfo("Ivan"));
  
//   TasksInfo updated_tasks, untouched_tasks;
  
//   tie(updated_tasks, untouched_tasks) =
//       tasks.PerformPersonTasks("Ivan", 2);
//   cout << "Updated Ivan's tasks: ";
//   PrintTasksInfo(updated_tasks);
//   cout << "Untouched Ivan's tasks: ";
//   PrintTasksInfo(untouched_tasks);
  
//   tie(updated_tasks, untouched_tasks) =
//       tasks.PerformPersonTasks("Ivan", 2);
//   cout << "Updated Ivan's tasks: ";
//   PrintTasksInfo(updated_tasks);
//   cout << "Untouched Ivan's tasks: ";
//   PrintTasksInfo(untouched_tasks);

//   return 0;
// }
