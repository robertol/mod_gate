class Test

  def initialize()
    @db = SQLite::DB.new()
    rc = @db.open("#{$modgate_test_dir}/files/foods.db")
    
    if rc != SQLITE_OK
      puts @db.error()
    end   
  end

  def run()
    native_query()
    row_query()
  end

  def native_query()
    query = @db.prepare('select * from foods')
    
    while query.step() == SQLITE_ROW
      query.each do |k,v|
        puts "  #{k}: #{v}"
      end    
    end
  end

  def row_query()
    sql = %Q{ select * from foods order by name }
    
    # Create a rowset
    rowset = SQLite::Rowset.new(@db)
    
    # Select everything from queues table
    rowset.select(sql)
    
    # Iterate and print
    rowset.each do |row|
      puts "%3i %2i %-30s" % [row[0], row[1], row[2]]
    end
  end
end

test = Test.new()
test.run()
