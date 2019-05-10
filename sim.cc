/* compile with: g++ -std=c++17  -Wpedantic -Wall -Wextra -Weffc++ -Werror -g -Ofast -march=native -mtune=native -o sim sim.cc */

#include <random>
#include <iostream>
#include <algorithm>

using namespace std;

double simulate_session( default_random_engine & rng,
			 const double session_duration,
			 const double mean_inter_stall_duration )
{
    exponential_distribution stall_duration_dist { 1.0 / 2.0 };
    exponential_distribution inter_stall_duration_dist { 1.0 / mean_inter_stall_duration };

    double cumulative_time_stalled = 0;
    double t = 0;
    bool playing = true;

    while ( t < session_duration ) {
	if ( playing ) {
	    t = t + inter_stall_duration_dist( rng );
	    playing = false;
	} else {
	    const double resume = min( session_duration, t + stall_duration_dist( rng ) );
	    cumulative_time_stalled += resume - t;
	    t = resume;
	    playing = true;
	}
    }

    return cumulative_time_stalled;
}

double simulate_day( default_random_engine & rng,
		     const unsigned int num_sessions,
		     const double mean_inter_stall_duration )
{
    exponential_distribution session_duration_dist { 1.0 / (10 * 60.0) };
    normal_distribution user_badness_dist( 1.0, 0.25 );

    double total_time = 0, time_stalled = 0;

    for ( unsigned int session = 0; session < num_sessions; session++ ) {
	const double session_duration = session_duration_dist( rng );
	total_time += session_duration;

	time_stalled += simulate_session( rng, session_duration, max( 10.0, mean_inter_stall_duration * user_badness_dist( rng ) ) );
    }

    return time_stalled / total_time;
}

int main()
{
    default_random_engine rng { random_device()() };

    const unsigned int num_days = 10000;

    uniform_int_distribution scheme_dist( 1, 10 );
    
    vector<double> test_statistics;
    for ( unsigned int day = 0; day < num_days; day++ ) {
	unsigned int puffer_sessions = 0, other_sessions = 0;

	for ( unsigned int session = 0; session < 5000; session++ ) {
	    int scheme = scheme_dist( rng );
	    if ( scheme == 1 ) {
		puffer_sessions++;
	    } else if ( scheme == 2 ) {
		other_sessions++;
	    }
	}

	const double puffer_stall_rate = simulate_day( rng, puffer_sessions, 798 );
	const double other_stall_rate = simulate_day( rng, other_sessions, 569 );
	
	test_statistics.push_back( other_stall_rate - puffer_stall_rate );
    }

    sort( test_statistics.begin(), test_statistics.end() );

    const double total = accumulate( test_statistics.begin(), test_statistics.end(), 0.0 );
	
    const double mean = total / double( num_days );
    
    cout.precision( 3 );

    //    const double median = test_statistics.at( .5 * num_days );
    const double p2_5 = test_statistics.at( .025 * num_days );
    const double p97_5 = test_statistics.at( .975 * num_days );

    cout << "Result: " << 100 * mean << " +/- "
	 << 100 * max( mean - p2_5, p97_5 - mean )
	 << "\n";

    unsigned int last_negative = 0;
    while ( test_statistics.at( last_negative ) < 0 ) {
	last_negative++;
    }

    cout << "Probability of wrong answer: " << 100 * last_negative / double( test_statistics.size() ) << "%\n";

    return 0;
}
