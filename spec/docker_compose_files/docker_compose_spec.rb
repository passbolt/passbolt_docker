describe 'docker-compose files' do

  let(:files) do
    [ 'docker-compose/docker-compose-ce.yaml',
    'docker-compose/docker-compose-pro.yaml',
    'docker-compose/docker-compose-ce-postgresql.yaml' ]
  end

  it 'contains the 80:8080 port forward commented for rootless images' do
    files.each do |file|
      expect(File.read("#{file}")).to match(/# - 80:8080/)
    end
  end

  it 'contains the 443:4433 port forward commented for rootless images' do
    files.each do |file|
      expect(File.read("#{file}")).to match(/# - 443:4433/)
    end
  end

  it 'contains the 443:433 port forward' do
    files.each do |file|
      expect(File.read("#{file}")).to match(/443:443/)
    end
  end

  it 'contains the 80:80 port forward' do
    files.each do |file|
      expect(File.read("#{file}")).to match(/80:80/)
    end
  end

end
