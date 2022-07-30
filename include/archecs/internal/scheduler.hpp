#pragma once

#include <vector>
#include <unordered_map>
#include <span>
#include "helper_macros.hpp"

namespace arch::det
{
	template<typename t_job>
	class scheduler
	{
	public:
		void add_job(t_job &job, std::span<t_job *> previous_jobs, std::span<t_job *> following_jobs)
		{
			const std::size_t job_id = get_job_id(job);
			
			for (t_job *dependency: previous_jobs)
			{
				std::size_t dep_id = get_job_id(*dependency);
				if (job_id != dep_id) [[likely]]
				{
					_jobs[dep_id].previous.push_back(job_id);
					_jobs[job_id].previous_count++;
				}
			}
			
			for (t_job *dependency: following_jobs)
			{
				std::size_t dep_id = get_job_id(*dependency);
				if (job_id != dep_id) [[likely]]
				{
					_jobs[job_id].previous.push_back(dep_id);
					_jobs[dep_id].previous_count++;
				}
			}
		}
		
		void clear()
		{
			_vertices.clear();
			_jobs.clear();
		}
		
		std::vector<t_job *> schedule_jobs()
		{
			// set tmp_dep_count
			for (job_node &node: _jobs)
			{
				node.tmp_previous_count = node.previous_count;
			}
			
			// implementation of kahn's algorithm
			std::vector<t_job *> resulting_order{};
			resulting_order.reserve(_jobs.size());
			
			// get nodes with no dependency
			std::vector<job_node *> start_nodes{};
			start_nodes.reserve(_jobs.size());
			for (std::size_t job_idx = 0; job_idx < _vertices.size(); ++job_idx)
			{
				job_node &current = _jobs[job_idx];
				if (current.tmp_previous_count == 0)
				{
					start_nodes.push_back(&current);
				}
			}
			
			for (std::size_t i = 0; i < start_nodes.size(); ++i)
			{
				job_node *start_job = start_nodes[i];
				resulting_order.push_back(start_job->job);
				
				for (std::size_t previous_job_id: start_job->previous)
				{
					auto &following_job = _jobs[previous_job_id];
					following_job.tmp_previous_count -= 1;
					if (following_job.tmp_previous_count == 0)
					{
						start_nodes.push_back(&following_job);
					}
				}
			}
			
			arch_assert_external("Dependencies contain at least one cycle" && start_nodes.size() == _jobs.size());
			
			return resulting_order;
		}
	
	private:
		struct job_node
		{
			t_job *job;
			std::size_t previous_count = 0;
			std::size_t tmp_previous_count = 0;
			std::vector<std::size_t> previous{};
		};
		
		[[nodiscard]]
		std::size_t get_job_id(t_job &job)
		{
			auto found = _vertices.find(&job);
			if (found == _vertices.end())
			{
				std::size_t job_idx = _jobs.size();
				_vertices[&job] = job_idx;
				
				_jobs.push_back({&job});
				
				return job_idx;
			}
			else
			{
				return found->second;
			}
		}
	
	private:
		std::unordered_map<t_job *, std::size_t> _vertices{};
		std::vector<job_node> _jobs;
	};
}