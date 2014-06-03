#!/usr/bin/ruby

require 'ipaddr'
require 'jw/dbi'

db = JW::DBI::Database.new()

db.open(db: '/etc/apache2/jerk.db', driver: 'QSQLITE')

q1 = db.query()
q2 = db.query()

q1.exec 'BEGIN EXCLUSIVE'

q1.exec 'select * from ip'

if q1.rows() == 0
  exit 0
end

q1.exec().each do |row|

  updates = {}
 
  if row['start'].size == 0
    updates['start'] = IPAddr.new(row['start_text']).to_i
  end

  if row['end'].size == 0
    updates['end'] = IPAddr.new(row['end_text']).to_i
  end

  id = row['id']

  if updates.size > 0
    fields = updates.to_a.collect {|x| "#{x[0]}=#{x[1]}"}.join(', ')
    q2.exec "update ip set #{fields} where id=#{id}"
  end
  
  q1.exec 'COMMIT'
end
