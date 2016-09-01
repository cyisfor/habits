This is a little thing that shows what habits you’re trying to do on a regular basis: brush your teeth, mow the lawn, call your grandmother, eat regular meals, take a break from the computer, don’t sit here for 10 hours programming a habits tracker, you know, habits.

tl;dr add habits with New, and give an interval in seconds (converted to milliseconds in the db). They pop up as bright red, and you go do them. Then you select them, and click “Did It.” Yay, you completed your checklist! It’ll remain empty until another one of your habits starts coming close to due again.

Building it, you’ll need gtk-3, libnotify, sqlite3, 

The habits are ordered according to which is the most “urgent” which is to say check out sql/pending.sql, but basically it’s the ratio of time elapsed since you last did it, to the frequency at which you do it. So once a day, it goes to the top after 2 days, then once an hour passes it in 2 hours.

They also start out green (and only pop up when you’re close to needing to do them), then slowly turn red as they become overdue. So basically, do the red ones, and those will be the ones on the top. If you’re sick of doing a habit and can’t stand to get up and do it one more second, click “Disabled” and it’ll be gone forever. (`sqlite3 habits.sqlite` to re-enable it though, I just never programmed any button to do it.)

It stores your history of when you did habits in the database, so you could do statistics on that. I haven’t done anything with that yet. Ideally the sum of the differences in times in history for a habit, divided by the number of history entries, should be equal to the frequency you set for that habit. It will tend to be higher though, because you’re probably more likely to occasionally take longer to do a habit, than to perform the habit more frequently than you planned.

 Importance gets stored, but currently means nothing.

