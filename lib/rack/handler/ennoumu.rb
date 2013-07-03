# coding: utf-8

require "rbconfig"

require_relative './ennou.rb'

module Rack
  module Handler
    class Ennoumu < Ennou

      QNAME = 'EnnouMu_Queue'

      @qname = nil
      @nprocs = 2
      @rackup = 'rackup'

      def self.config(options)
        set_option(:@qname, options[:qname])
        set_option(:@nprocs, options[:nprocs])
      end

      def self.set_option(v, s)
        instance_variable_set(v, s) if s
      end
        
      def self.run(app, options = {})
        setup(options)
        ::Ennou::Server.open(create_qname, true) do |server|
          @server = server
          if server.controller?
            @stoprun = false
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} start on #{RUBY_VERSION}(#{RUBY_PLATFORM})"
            if @script == ''
              server.add "http://#{@host}:#{@port}/"
            else
              server.add "http://#{@host}:#{@port}#{@script}/"
            end
            pids = []
            cmd = "#{::File.expand_path('ruby.exe', RbConfig::CONFIG["bindir"])} #{::File.expand_path("../#{@rackup}", $0)} #{$DEBUG ? '-d' : ''} #{$VERBOSE ? '-w' : ''} #{options[:Host] == '0.0.0.0' ? '' : "-o #{@host}"} -p #{@port} -s Ennoumu \"#{options[:config]}\""
            1.upto(@nprocs) do
              pids << spawn(cmd)
              @logger.info " spawn worker pid=#{pids.last}"
            end
            until @stoprun do
              chld = Process.wait
              pids.reject! {|pid| pid == chld}
              if $?.exitstatus == 2
                # spawn the new worker process(ad hoc)
                pids << spawn(cmd)
                @logger.info " spawn worker pid=#{pids.last}"
                STDOUT.print "\n"
              end
            end  
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} stop"
            Process.waitall
          else
            server.script =  @script
            @logger.info "script=#{server.script}, #{@script}"
            @logger.info "Ennou(#{::Ennou::VERSION}) start for http://#{@host}:#{@port}#{@script} pid=#{$$}"
            ret = 0
            loop do
              begin
                r = server.wait(60)
                next if r.nil?
                run_thread(app, *r)
              rescue Interrupt
                break
              rescue => e
                ret = 2
                break
              end
            end
            @logger.info "Ennou(#{::Ennou::VERSION}) stop service for http://#{@host}:#{@port}#{@script} pid=#{$$}"
            exit ret
          end
        end
      end   

      def self.multiprocess?
        true
      end

      def self.shutdown
        if @server.controller?
          @stoprun = true
        else
          @server.break
          @logger.info "going to shutdown ... pid=#{$$}"
        end
      end
      
      private

      def self.create_qname
        qname = "#{QNAME}_#{(@qname.nil?) ? ((@script == '') ? @host : @script) : @qname}".gsub('/', '')
        @logger.info 'EnnouMu qname=' + qname
        qname
      end
      
    end
  end    
end
